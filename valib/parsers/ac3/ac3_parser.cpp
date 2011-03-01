#include <sstream>
#include <iomanip>
#include "../../crc.h"
#include "ac3_header.h"
#include "ac3_parser.h"
#include "ac3_bitalloc.h"
#include "ac3_tables.h"

// todo:
// * crc check (start_decode)
// * verify bit allocation conditions:
//   do we need to make _all_ bit allocation if deltbaie = 1?
// * grouped mantissas verification

#define EXP_REUSE 0
#define EXP_D15   1
#define EXP_D25   2
#define EXP_D45   3

#define DELTA_BIT_REUSE    0
#define DELTA_BIT_NEW      1
#define DELTA_BIT_NONE     2
#define DELTA_BIT_RESERVED 3



///////////////////////////////////////////////////////////////////////////////
// AC3Parser
///////////////////////////////////////////////////////////////////////////////

AC3Parser::AC3Parser()
{
  frames = 0;
  errors = 0;

  do_crc = true;
  do_dither = true;
  do_imdct = true;

  // allocate buffers
  samples.allocate(AC3_NCHANNELS, AC3_FRAME_SAMPLES);
  delay.allocate(AC3_NCHANNELS, AC3_BLOCK_SAMPLES);
  header.allocate(ac3_header.header_size());

  reset();
}

///////////////////////////////////////////////////////////////////////////////
// SimpleFilter overrides

bool
AC3Parser::can_open(Speakers spk) const
{
  return spk.format == FORMAT_AC3;
}

bool
AC3Parser::init()
{
  reset();
  return true;
}

void 
AC3Parser::reset()
{
  memset((AC3Info*)this, 0, sizeof(AC3Info));
  memset((AC3FrameState*)this, 0, sizeof(AC3FrameState));

  out_spk = spk_unknown;
  frame = 0;
  frame_size = 0;
  bs_type = 0;
  new_stream_flag = false;
  header.zero();

  block = 0;
  samples.zero();
  delay.zero();
  lfsr_state = 1;
}

bool
AC3Parser::process(Chunk &in, Chunk &out)
{
  bool sync = in.sync;
  vtime_t time = in.time;
  in.set_sync(false, 0);

  if (in.size == 0)
    return false;

  if (!parse_frame(in.rawdata, in.size))
  {
    in.clear();
    errors++;
    return false;
  }

  if (ac3_header.compare_headers(in.rawdata, header))
    new_stream_flag = false;
  else
  {
    new_stream_flag = true;
    memcpy(header, in.rawdata, ac3_header.header_size());
  }

  in.clear();
  out.set_linear(samples, AC3_FRAME_SAMPLES, sync, time);
  frames++;
  return true;
}

string
AC3Parser::info() const 
{
  using std::endl;
  int max_freq = (cplinu? MAX(endmant[0], cplendmant): endmant[0]) * out_spk.sample_rate / 512000;
  int cpl_freq = cplinu? cplstrtmant * out_spk.sample_rate / 512000: max_freq;

  std::stringstream result;
  result << "Format: " << out_spk.print() << endl;
  result << "Bitrate: " << bitrate << endl;
  result << "Stream: " << (bs_type == BITSTREAM_8? "8 bit": "16bit low endian") << endl;
  result << "Frame size: " << frame_size << endl;
  result << "NSamples: " << AC3_FRAME_SAMPLES << endl;
  result << "bsid: " << bsid << endl;
  result << "clev: " << std::setprecision(1) << value2db(clev) << "dB (" << std::setprecision(4) << clev << ")" << endl;
  result << "slev: " << std::setprecision(1) << value2db(slev) << "dB (" << std::setprecision(4) << slev << ")" << endl;
  result << "dialnorm: " << -dialnorm << "dB" << endl;
  result << "bandwidth: " << cpl_freq << "kHz/" << max_freq << "kHz" << endl;
  return result.str();
}

///////////////////////////////////////////////////////////////////////////////
// AC3 parse

bool
AC3Parser::parse_frame(uint8_t *_frame, size_t _size)
{
  HeaderInfo hinfo;

  if (_size < ac3_header.header_size())
    return false;

  if (!ac3_header.parse_header(_frame, &hinfo))
    return false;

  if (hinfo.frame_size > _size)
    return false;

  out_spk = hinfo.spk;
  out_spk.format = FORMAT_LINEAR;
  frame = _frame;
  frame_size = hinfo.frame_size;
  bs_type = hinfo.bs_type;

  if (bs_type != BITSTREAM_8)
    if (bs_convert(frame, _size, bs_type, frame, BITSTREAM_8) == 0)
      return false;

  if (do_crc)
    if (!crc_check())
      return false;

  bs.set(frame, 0, frame_size * 8);

  if (!parse_header())
    return false;

  while (block < AC3_NBLOCKS)
    if (!decode_block())
      return false;

  return true;
}

bool
AC3Parser::crc_check()
{
  // Note: AC3 uses standard CRC16 polinomial

  uint32_t crc;

  /////////////////////////////////////////////////////////
  // Check first 5/8 of frame
  // CRC is initialized by 0 and test result must be also 0.
  // Syncword (first 2 bytes) is not imcluded to crc calc
  // but it is included to 5/8 of frame size. So we must 
  // check 5/8*frame_size - 2 bytes.

  size_t frame_size1 = ((frame_size >> 2) + (frame_size >> 4)) << 1;
  crc = crc16.calc(0, frame + 2, frame_size1 - 2);
  if (crc) 
    return false;

  /////////////////////////////////////////////////////////
  // Check the rest of frame
  // CRC is initialized by 0 (from previous point) and test
  // result must be also 0.

  crc = crc16.calc(0, frame + frame_size1, frame_size - frame_size1);
  if (crc) 
    return false;

  return true;
}

bool 
AC3Parser::parse_header()
// Fill AC3Info structure
{
  /////////////////////////////////////////////////////////////
  // Skip syncword

  bs.get(32);

  /////////////////////////////////////////////////////////////
  // Parse bit stream information (BSI)

  fscod      = bs.get(2);            // 'fscod' - sample rate code
  frmsizecod = bs.get(6);            // 'frmsizecod' - frame size code
  bsid       = bs.get(5);            // 'bsid' - bitstream identification
  bsmod      = bs.get(3);            // 'bsmod' - bitstreeam mode
  acmod      = bs.get(3);            // 'acmod' - audio coding mode

  halfrate = halfrate_tbl[bsid];
  bitrate = bitrate_tbl[frmsizecod >> 1];

  if ((acmod & 1) && (acmod != 1))
    clev = clev_tbl[bs.get(2)];      // 'clev' - center mix level
  else
    clev = 1.0;

  if (acmod & 4)
    slev = slev_tbl[bs.get(2)];      // 'slev' - surround mix level
  else
    slev = 1.0;

  if (acmod == AC3_MODE_STEREO)
    dsurmod  = bs.get(2);            // 'dsurmod' - Dolby Surround mode
  else
    dsurmod  = 0;

  lfeon      = bs.get_bool();        // 'lfeon' - flag shows if it is LFE channel in stream
  dialnorm   = bs.get(5);            // 'dialnorm' - dialog normalization

  compre     = bs.get_bool();        // 'compre' - compression gain word
  if (compre)
    compr    = bs.get(8);            // 'compr' - compression gain word
  else
    compr    = 0;

  langcode   = bs.get_bool();        // 'langcode' - language code exists
  if (langcode)
    langcod  = bs.get(8);            // 'langcod' - language code
  else
    langcod  = 0;
                                     
  audprodie  = bs.get_bool();        // 'audprodie' - audio production information exists
  if (audprodie)
  {
    mixlevel = bs.get(5) + 80;       // 'mixlevel' - mixing level in SPL
    roomtyp  = bs.get(2);            // 'roomtyp' - room type
  }
  else
  {
    mixlevel = 0;
    roomtyp  = 0;
  }

  if (acmod == AC3_MODE_DUAL)            
  {                                  
    dialnorm2  = bs.get(5);          // 'dialnorm2' - dialog normalization
                                     
    compr2e    = bs.get_bool();      // 'compr2e' - compression gain word
    if (compr2e)                     
      compr2   = bs.get(8);          // 'compr2' - compression gain word
    else
      compr2   = 0;
                                     
    langcod2e  = bs.get_bool();      // 'langcod2e' - language code exists
    if (langcod2e)
      langcod  = bs.get(8);          // 'langcod2' - language code
    else
      langcod  = 0;

    audprodi2e = bs.get_bool();      // 'audprodi2e' - audio production information exists
    if (audprodi2e)
    {
      mixlevel2 = bs.get(5) + 80;    // 'mixlevel2' - mixing level in SPL
      roomtyp2  = bs.get(2);         // 'roomtyp2' - room type
    }
    else
    {
      mixlevel2 = 0;
      roomtyp2  = 0;
    }
  }
  else
  {
    dialnorm2  = 0;
    compr2e    = false;
    compr2     = 0;
    langcod2e  = false;
    langcod    = 0;
    audprodi2e = false;
    mixlevel2  = 0;
    roomtyp2   = 0;
  }

  copyrightb = bs.get_bool();        // 'copyrightb' - copyright bit
  origbs     = bs.get_bool();        // 'origbs' - original bitstream

  if (bs.get_bool())                 // 'timecod1e' - timecode first half exists
  {
    timecode.hours = bs.get(5);
    timecode.mins  = bs.get(6);
    timecode.secs  = bs.get(3) << 4;
  }

  if (bs.get_bool())                 // 'timecod2e' - timecode second half exists
  {
    timecode.secs  += bs.get(3);
    timecode.frames = bs.get(5);
    timecode.fracs  = bs.get(6);
  }
  
  if (bs.get_bool())                 // 'addbsie' - additional bitstream information exists
  {
    int addbsil = bs.get(6);         // 'addbsil' - additioanl bitstream information length
    while (addbsil--)
      bs.get(8);                     // 'addbsi' - additional bitstream information
  }

  /////////////////////////////////////////////////////////////
  // Init variables to for first block decoding

  block    = 0;

  dynrng   = 1.0;
  dynrng2  = 1.0;

  cpldeltbae = DELTA_BIT_NONE;
  deltbae[0] = DELTA_BIT_NONE;
  deltbae[1] = DELTA_BIT_NONE;
  deltbae[2] = DELTA_BIT_NONE;
  deltbae[3] = DELTA_BIT_NONE;
  deltbae[4] = DELTA_BIT_NONE;

  return true;
}

bool 
AC3Parser::decode_block()
{
  samples_t d = delay;
  samples_t s = samples;
  s += (block * AC3_BLOCK_SAMPLES);

  if (block >= AC3_NBLOCKS || !parse_block())
  {
    block = AC3_NBLOCKS; // prevent further decoding
    return false;
  }
  parse_coeff(s);

  if (do_imdct)
  {
    int nfchans = out_spk.lfe()? out_spk.nch() - 1: out_spk.nch();
    for (int ch = 0; ch < nfchans; ch++)
      if (blksw[ch])
        imdct.imdct_256(s[ch], delay[ch]);
      else
        imdct.imdct_512(s[ch], delay[ch]);

    if (out_spk.lfe())
      imdct.imdct_512(s[nfchans], d[nfchans]);
  }

  block++;
  return true;
}

bool
AC3Parser::parse_block()
{
  int nfchans = nfchans_tbl[acmod];
  int ch, bnd;

  ///////////////////////////////////////////////
  // bit allocation bitarray; bits are:
  // 0-4 = do bit allocation for fbw channels
  // 5   = do bit allocation for lfe channel
  // 6   = do bit allocation for coupling channel
  int bitalloc = 0;

  for (ch = 0; ch < nfchans; ch++)
    blksw[ch] = bs.get_bool();                // 'blksw[ch]' - block switch flag

  for (ch = 0; ch < nfchans; ch++)
    dithflag[ch] = bs.get_bool();             // 'dithflag[ch]' - dither flag

  // reset dithering info 
  // if we do not want to dither
  if (!do_dither)
    for (ch = 0; ch < nfchans; ch++)
      dithflag[ch] = 0;

  if (bs.get_bool())                          // 'dynrnge' - dynamic range gain word exists
  {
    int32_t dynrng_word = bs.get_signed(8);   // 'dynrng' - dynamic range gain word
    dynrng = (((dynrng_word & 0x1f) | 0x20) << 13) * scale_factor[(3 - (dynrng_word >> 5)) & 7];
  }

  if (acmod == AC3_MODE_DUAL)
    if (bs.get_bool())                        // 'dynrng2e' - dynamic range gain word 2 exists
    {
      int32_t dynrng_word = bs.get_signed(8); // 'dynrng2' - dynamic range gain word 2 
      dynrng2 = (((dynrng_word & 0x1f) | 0x20) << 13) * scale_factor[(3 - (dynrng_word >> 5)) & 7];
    }

  /////////////////////////////////////////////////////////
  // Coupling strategy information
  /////////////////////////////////////////////////////////

  if (bs.get_bool())                          // 'cplstre' - coupling strategy exists
  {
    cplinu = bs.get_bool();                   // 'cplinu' - coupling in use
    if (cplinu)
    {
      if (acmod == AC3_MODE_MONO || acmod == AC3_MODE_DUAL)
        return false;                         // this modes are not allowed for coupling
                                              // constraint p...
      for (ch = 0; ch < nfchans; ch++)
        chincpl[ch] = bs.get_bool();          // 'chincpl[ch]' - channel in coupliing

      if (acmod == AC3_MODE_STEREO)
        phsflginu = bs.get_bool();            // 'phsflginu' - phase flags in use

      int cplbegf = bs.get(4);                // 'cplbegf' - coupling begin frequency code
      int cplendf = bs.get(4);                // 'cplendf' - coupling end frequency code

      int ncplsubnd = cplendf - cplbegf + 3;
      if (ncplsubnd < 0)
        return false;                         // constraint p...

      cplstrtmant = cplbegf * 12 + 37;
      cplendmant  = cplendf * 12 + 73;

      ncplbnd = 0;
      cplbnd[0] = cplstrtmant + 12;
      for (bnd = 0; bnd < ncplsubnd - 1; bnd++)
        if (bs.get_bool())                    // 'cplbndstrc[bnd]' - coupling band structure
          cplbnd[ncplbnd] += 12;
        else
        {
          ncplbnd++;
          cplbnd[ncplbnd] = cplbnd[ncplbnd - 1] + 12;
        }
      ncplbnd++; // coupling band index to number to coupling bands
    }
    else
    {
      chincpl[0] = false;
      chincpl[1] = false;
      chincpl[2] = false;
      chincpl[3] = false;
      chincpl[4] = false;
    }
  }
  else // if (bs.get_bool())                  // 'cplstre' - coupling strategy exists
    if (!block)                               // cplstre <> 0 for block 0 (constraint p39 s5.4.3.7)
      return false;

  /////////////////////////////////////////////////////////
  // Coupling coordinates
  // todo: constraint p.41 s5.4.3.14
  /////////////////////////////////////////////////////////

  if (cplinu)
  {
    bool cplcoe = false;
    int  mstrcplco;
    int  cplcoexp;
    int  cplcomant;

    for (ch = 0; ch < nfchans; ch++)
      if (chincpl[ch])
        if (bs.get_bool())                    // 'cplcoe[ch]' coupling coordinates exists
        {
          cplcoe = true;

          mstrcplco = bs.get(2) * 3;          // 'mstrcplco' - master coupling coordinate
          for (bnd = 0; bnd < ncplbnd; bnd++)
          {
            cplcoexp  = bs.get(4);            // 'cplcoexp' - coupling coordinate exponent
            cplcomant = bs.get(4);            // 'cplcomant' - coupling coordinate mantissa

            if (cplcoexp == 15)
              cplcomant <<= 14;
            else
              cplcomant = (cplcomant | 0x10) << 13;

            cplco[ch][bnd] = cplcomant * scale_factor[cplcoexp + mstrcplco];
          } // for (int bnd = 0; bnd < ncplbnd; bnd++)
        } // if (bs.get_bool())               // 'cplcoe[ch]' coupling coordinates exists

    if (acmod == AC3_MODE_STEREO && phsflginu && cplcoe)
      for (bnd = 0; bnd < ncplbnd; bnd++)
        if (bs.get_bool())                    // 'phsflg' - phase flag
          cplco[1][bnd] = -cplco[1][bnd];
  }

  /////////////////////////////////////////////////////////
  // Rematrixing
  /////////////////////////////////////////////////////////

  if (acmod == AC3_MODE_STEREO)
    if (bs.get_bool())                        // 'rematstr' - rematrixing strategy
    {
      bnd = 0;
      rematflg = 0;
      int endbin = cplinu? cplstrtmant: 253;
      do
        rematflg |= bs.get(1) << bnd;         // rematflg[bnd] - rematrix flag
      while (rematrix_tbl[bnd++] < endbin);
    }
/*
    This check is disabled because some buggy encoder exists that
    breaks this rule.

    else if (block == 0)                      // rematstr <> 0 for block 0 (constraint p41 s5.4.3.19)
      return false;
*/
  /////////////////////////////////////////////////////////
  // Exponents
  /////////////////////////////////////////////////////////

  int cplexpstr;
  int chexpstr[5];
  int lfeexpstr;

  if (cplinu)
  {
    cplexpstr = bs.get(2);                    // 'cplexpstr' coupling exponent strategy
    if (cplexpstr == EXP_REUSE && block == 0) // cplexpstr <> reuse in block 0 (constraint p42 s5.4.3.21)
      return false;
  }

  for (ch = 0; ch < nfchans; ch++)
  {
    chexpstr[ch] = bs.get(2);                 // 'chexpstr[ch]' - channel exponent strategy
    if (chexpstr[ch] == EXP_REUSE && block == 0) // chexpstr[ch] <> reuse in block 0 (constraint p42 s5.4.3.22)
      return false;
  }

  if (lfeon)
  {
    lfeexpstr = bs.get(1);                    // 'lfeexpstr' - LFE exponent strategy
    if (lfeexpstr == EXP_REUSE && block == 0) // lfeexpstr <> reuse in block 0 (constraint p42 s5.4.3.23)
      return false;
  }

  for (ch = 0; ch < nfchans; ch++)
    if (chexpstr[ch] != EXP_REUSE)
      if (chincpl[ch])
        endmant[ch] = cplstrtmant;
      else
      {
        int chbwcod = bs.get(6);              // 'chbwcod[ch]' - channel bandwidth code

        if (chbwcod > 60)
          return false;                       // chbwcod[ch] <= 60 (constraint p42 s5.4.3.24)

        endmant[ch] = chbwcod * 3 + 73;
      }

  if (cplinu && cplexpstr != EXP_REUSE)
  {
    bitalloc |= 1 << 6; // do bit allocation for coupling channel
    int ncplgrps = (cplendmant - cplstrtmant) / (3 << (cplexpstr - 1));
    int8_t cplabsexp = bs.get(4) << 1;
    if (!parse_exponents(cplexps + cplstrtmant, cplabsexp, cplexpstr, ncplgrps))
      return false;
  }

  for (ch = 0; ch < nfchans; ch++)
    if (chexpstr[ch] != EXP_REUSE)
    {
      bitalloc |= 1 << ch; // do bit allocation for channel ch
      int nexpgrps;
      switch (chexpstr[ch])
      {
        case EXP_D15: nexpgrps = (endmant[ch] - 1)     / 3;  break;
        case EXP_D25: nexpgrps = (endmant[ch] - 1 + 3) / 6;  break;
        case EXP_D45: nexpgrps = (endmant[ch] - 1 + 9) / 12; break;
      }

      exps[ch][0] = bs.get(4);
      if (!parse_exponents(exps[ch] + 1, exps[ch][0], chexpstr[ch], nexpgrps))
        return false;

      gainrng[ch] = bs.get(2);                // 'gainrng[ch]' - gain range code
    }

  if (lfeon && lfeexpstr != EXP_REUSE)
  {
    bitalloc |= 1 << 5; // do bit allocation for lfe channel
    lfeexps[0] = bs.get(4);
    if (!parse_exponents(lfeexps + 1, lfeexps[0], lfeexpstr, 2))
      return false;
  }

  /////////////////////////////////////////////////////////
  // Bit allocation parametric information
  /////////////////////////////////////////////////////////

  if (bs.get_bool())                          // 'baie' - bit allocation information exists
  {
    bitalloc |= -1; // do all bit allocation
    sdecay = sdecay_tbl[bs.get(2)];           // 'sdcycod' - slow decay code
    fdecay = fdecay_tbl[bs.get(2)];           // 'fdcycod' - fast decay code
    sgain  = sgain_tbl[bs.get(2)];            // 'sgaincod' - slow gain code
    dbknee = dbknee_tbl[bs.get(2)];           // 'dbpbcod' - dB per bit code
    floor  = floor_tbl[bs.get(3)];            // 'floorcod' - masking floor code
  }
  else // if (bs.get_bool())                  // 'baie' - bit allocation information exists
    if (block == 0)                           // baie <> 0 in block 0 (constraint p43 s5.4.3.30)
      return false;

  if (bs.get_bool())                          // 'snroffste' - SNR offset exists
  {
    bitalloc |= -1; // do all bit allocation
    int csnroffst = bs.get(6);                // 'csnroffst' - coarse SNR offset
    if (cplinu)
    {
      int cplfsnroffst = bs.get(4);           // 'cplfsnroffst' - coupling fine SNR offset
      cplsnroffset = (((csnroffst - 15) << 4) + cplfsnroffst) << 2;
      cplfgain = fgain_tbl[bs.get(3)];        // 'cplfgaincod' - coupling fast gain code
    }

    for (ch = 0; ch < nfchans; ch++)
    {      
      int fsnroffst = bs.get(4);              // 'fsnroffst' - channel fine SNR offset
      snroffset[ch] = (((csnroffst - 15) << 4) + fsnroffst) << 2;
      fgain[ch] = fgain_tbl[bs.get(3)];       // 'fgaincod' - channel fast gain code
    }

    if (lfeon)
    {      
      int lfefsnroffst = bs.get(4);           // 'lfesnroffst' - LFE channel SNR offset
      lfesnroffset = (((csnroffst - 15) << 4) + lfefsnroffst) << 2;
      lfefgain = fgain_tbl[bs.get(3)];        // 'lfegaincod' - LFE channel gain code
    }
  }
  else // if (bs.get_bool())                  // 'snroffste' - SNR offset exists
    if (block == 0)                           // snroffte <> 0 in block 0 (constraint p44 s5.4.3.36)
      return false;

  if (cplinu)                                 
    if (bs.get_bool())                        // 'cplleake' - coupling leak initalization exists
    {
      bitalloc |= 1 << 6; // do bit allocations for coupling channel
      cplfleak = (bs.get(3) << 8) + 768;      // 'cplfleak' - coupling fast leak initialization
      cplsleak = (bs.get(3) << 8) + 768;      // 'cplsleak' - coupling slow leak initialization
    }
    else // if (bs.get_bool())                // 'cplleake' - coupling leak initalization exists
      if (block == 0)                         // cplleake <> 0 in block 0 (constraint p44 s5.4.3.44)
        return false;

  /////////////////////////////////////////////////////////
  // Delta bit allocation information
  /////////////////////////////////////////////////////////

  if (bs.get_bool())                          // 'deltbaie' - delta bit information exists
  {
    bitalloc |= -1; // do all bit allocation?

    if (cplinu)
    {
      cpldeltbae = bs.get(2);                 // 'cpldeltbae' - coupling delta bit allocation exists
      if (cpldeltbae == DELTA_BIT_REUSE && block == 0)
        return false;                         // cpldeltbae <> 0 in block 0 (constraint p45 s5.4.3.48)
    }

    for (ch = 0; ch < nfchans; ch++)
    {
      deltbae[ch] = bs.get(2);                // 'deltbae[ch]' - delta bit allocation exists
      if (deltbae == DELTA_BIT_REUSE && block == 0)
        return false;                         // deltbae[ch] <> 0 in block 0 (constraint p45 s5.4.3.49)
    }

    if (cplinu && cpldeltbae == DELTA_BIT_NEW)
      if (!parse_deltba(cpldeltba))
        return false;

    for (ch = 0; ch < nfchans; ch++)
      if (deltbae[ch] == DELTA_BIT_NEW)
        if (!parse_deltba(deltba[ch]))
          return false;
  }

  /////////////////////////////////////////////////////////
  // Skip data
  /////////////////////////////////////////////////////////

  if (bs.get_bool())
  {
    int skipl = bs.get(9);
    while (skipl--)
      bs.get(8);
  }

  /////////////////////////////////////////////////////////
  // Do bit allocation
  /////////////////////////////////////////////////////////

  if (bitalloc)
  {
    bool got_cplchan = false;
    BAP_BitCount counter;

    for (ch = 0; ch < nfchans; ch++)
    {
      if (bitalloc & (1 << ch))
      {
        bit_alloc(
          bap[ch], exps[ch],
          deltbae[ch], deltba[ch],
          0, endmant[ch], 
          fscod, halfrate, 
          sdecay, fdecay, 
          sgain, fgain[ch], 
          dbknee, floor, 
          0, 0, 
          snroffset[ch]);

        counter.add_bap(bap[ch], 0, endmant[ch]);
      }

      if (cplinu && !got_cplchan && (bitalloc & (1 << 6)))
      {
        got_cplchan = true;
        bit_alloc(
          cplbap, cplexps, 
          cpldeltbae, cpldeltba,
          cplstrtmant, cplendmant, 
          fscod, halfrate, 
          sdecay, fdecay, 
          sgain, cplfgain, 
          dbknee, floor, 
          cplfleak, cplsleak, 
          cplsnroffset);

        counter.add_bap(cplbap, cplstrtmant, cplendmant);
      }
    }

    if (lfeon && bitalloc & (1 << 5))
    {
      bit_alloc(
        lfebap, lfeexps,
        DELTA_BIT_NONE, 0,
        0, 7, 
        fscod, halfrate, 
        sdecay, fdecay, 
        sgain, lfefgain, 
        dbknee, floor, 
        0, 0, 
        lfesnroffset);

      counter.add_bap(lfebap, 0, 7);
    }

    if (bs.get_pos_bits() + counter.bits > frame_size * 8)
      return false;
  }

  return true;
}

bool
AC3Parser::parse_exponents(int8_t *exps, int8_t absexp, int expstr, int nexpgrps)
{
  int expgrp;

  switch (expstr)
  {
  case EXP_D15:
    while (nexpgrps--)
    {
      expgrp = bs.get(7);

      absexp += exp1_tbl[expgrp];
      if (absexp > 24) return false;
      *(exps++) = absexp;

      absexp += exp2_tbl[expgrp];
      if (absexp > 24) return false;
      *(exps++) = absexp;

      absexp += exp3_tbl[expgrp];
      if (absexp > 24) return false;
      *(exps++) = absexp;
    }
    break;

  case EXP_D25:
    while (nexpgrps--)
    {
      expgrp = bs.get(7);

      absexp += exp1_tbl[expgrp];
      if (absexp > 24) return false;
      *(exps++) = absexp;
      *(exps++) = absexp;

      absexp += exp2_tbl[expgrp];
      if (absexp > 24) return false;
      *(exps++) = absexp;
      *(exps++) = absexp;

      absexp += exp3_tbl[expgrp];
      if (absexp > 24) return false;
      *(exps++) = absexp;
      *(exps++) = absexp;
    }
    break;

  case EXP_D45:
    while (nexpgrps--)
    {
      expgrp = bs.get(7);
      if (expgrp >= 125) 
        return false;

      absexp += exp1_tbl[expgrp];
      if (absexp > 24) return false;
      *(exps++) = absexp;
      *(exps++) = absexp;
      *(exps++) = absexp;
      *(exps++) = absexp;

      absexp += exp2_tbl[expgrp];
      if (absexp > 24) return false;
      *(exps++) = absexp;
      *(exps++) = absexp;
      *(exps++) = absexp;
      *(exps++) = absexp;

      absexp += exp3_tbl[expgrp];
      if (absexp > 24) return false;
      *(exps++) = absexp;
      *(exps++) = absexp;
      *(exps++) = absexp;
      *(exps++) = absexp;
    }
    break;
  }

  return true;
}

bool
AC3Parser::parse_deltba(int8_t *deltba)
{
  int deltnseg, deltlen, delta, band;

  memset(deltba, 0, 50);

  deltnseg = bs.get(3) + 1;                   // 'cpldeltnseg'/'deltnseg'' - coupling/channel delta bit allocation number of segments
  band = 0;
  while (deltnseg--)
  {
    band += bs.get(5);                        // 'cpldeltoffst'/'deltoffst' - coupling/channel delta bit allocation offset
    deltlen = bs.get(4);                      // 'cpldeltlen'/'deltlen' - coupling/channel delta bit allocation length
    delta = bs.get(3);                        // 'cpldeltba'/'deltba' - coupling/channel delta bit allocation

    if (delta >= 4)
      delta = (delta - 3) << 7;
    else
      delta = (delta - 4) << 7;

    if (band + deltlen >= 50)
      return false;

    while (deltlen--)
      deltba[band++] = delta;
  }

  return true;
}

void 
AC3Parser::parse_coeff(samples_t samples)
{
  int ch, bnd, s;
  Quantizer q;

  int nfchans = nfchans_tbl[acmod];
  bool got_cplchan = false;

  /////////////////////////////////////////////////////////////
  // Get coeffs

  for (ch = 0; ch < nfchans; ch++)
  {
    // parse channel mantissas
    get_coeff(q, samples[ch], bap[ch], exps[ch], endmant[ch], dithflag[ch]);

    if (chincpl[ch] && !got_cplchan)
    {
      // parse coupling channel mantissas
      got_cplchan = true;
      get_coeff(q, samples[ch] + cplstrtmant, cplbap + cplstrtmant, cplexps + cplstrtmant, cplendmant - cplstrtmant, false);

      // copy coupling coeffs to all coupled channels
      for (int ch2 = ch + 1; ch2 < nfchans; ch2++)
        if (chincpl[ch2])
          memcpy(samples[ch2] + cplstrtmant, samples[ch] + cplstrtmant, (cplendmant - cplstrtmant) * sizeof(sample_t));
    }
  }

  if (lfeon)
  {
    get_coeff(q, samples[nfchans], lfebap, lfeexps, 7, false);
    memset(samples[nfchans] + 7, 0, 249 * sizeof(sample_t));
  }

  // Dither
  for (ch = 0; ch < nfchans; ch++)
    if (chincpl[ch] && dithflag[ch])
      for (s = cplstrtmant; s < cplendmant; s++)
        if (!cplbap[s])
          samples[ch][s] = dither_gen() * scale_factor[cplexps[s]];

  // Apply coupling coordinates
  for (ch = 0; ch < nfchans; ch++)
    if (chincpl[ch])
    {
      s = cplstrtmant;
      for (bnd = 0; bnd < ncplbnd; bnd++)
        while (s < cplbnd[bnd])
          samples[ch][s++] *= cplco[ch][bnd];
    }

  // Clear tails
  for (ch = 0; ch < nfchans; ch++)
    if (chincpl[ch])
      memset(samples[ch] + cplendmant, 0, (256 - cplendmant) * sizeof(sample_t));
    else
      memset(samples[ch] + endmant[ch], 0, (256 - endmant[ch]) * sizeof(sample_t));

  /////////////////////////////////////////////////////////////
  // Rematrixing

  if (acmod == AC3_MODE_STEREO) 
  {
    int bin = 13;
    int bnd = 0;
    int band_end = 0;
    int last_bin = MIN(endmant[0], endmant[1]);
    int remat = rematflg;
    do
    {
      if (!(remat & 1))
      {
        remat >>= 1;
        bin = rematrix_tbl[bnd++];
        continue;
      }
      remat >>= 1;
      band_end = rematrix_tbl[bnd++];

      if (band_end > last_bin)
        band_end = last_bin;

      do 
      {
        sample_t tmp0 = samples[0][bin];
        sample_t tmp1 = samples[1][bin];
        samples[0][bin] = tmp0 + tmp1;
        samples[1][bin] = tmp0 - tmp1;
      } while (++bin < band_end);
    } while (bin < last_bin);
  }
}

void 
AC3Parser::get_coeff(Quantizer &q, sample_t *s, int8_t *bap, int8_t *exp, int n, bool dither)
{
  int ibap;
  while (n--)
  {
    ibap = *bap++;
    switch (ibap)
    {
      case 0:
        if (dither)
          *s++ = dither_gen() * scale_factor[*exp++];
        else
        {
          *s++ = 0;
          exp++;
        }
        break;

      case 1: 
        // 3-levels 3 values in 5 bits
        if (q.q3_cnt--)
          *s++ = q.q3[q.q3_cnt] * scale_factor[*exp++];
        else
        {
          int code = bs.get(5);
          q.q3[0] = q3_3_tbl[code];
          q.q3[1] = q3_2_tbl[code];
          q.q3_cnt = 2;
          *s++ = q3_1_tbl[code] * scale_factor[*exp++];
        }
        break;

      case 2:  
        // 5-levels 3 values in 7 bits
        if (q.q5_cnt--)
          *s++ = q.q5[q.q5_cnt] * scale_factor[*exp++];
        else
        {
          int code = bs.get(7);
          q.q5[0] = q5_3_tbl[code];
          q.q5[1] = q5_2_tbl[code];
          q.q5_cnt = 2;
          *s++ = q5_1_tbl[code] * scale_factor[*exp++];
        }
        break;

      case 3:
        *s++ = q7_tbl[bs.get(3)] * scale_factor[*exp++];
        break;

      case 4:
        // 11-levels 2 values in 7 bits
        if (q.q11_cnt--)
          *s++ = q.q11 * scale_factor[*exp++];
        else
        {
          int code = bs.get(7);
          q.q11 = q11_2_tbl[code];
          q.q11_cnt = 1;
          *s++ = q11_1_tbl[code] * scale_factor[*exp++];
        }
        break;

      case 5:
        *s++ = q15_tbl[bs.get(4)] * scale_factor[*exp++];
        break;

      case 14:       
        *s++ = (bs.get_signed(14) << 2) * scale_factor[*exp++];
        break;

      case 15: 
        *s++ = bs.get_signed(16) * scale_factor[*exp++];
        break;

      default: 
        *s++ = (bs.get_signed(ibap - 1) << (16 - (ibap - 1))) * scale_factor[*exp++];
        break;
    }
  }
}

int16_t
AC3Parser::dither_gen()
{
  static const uint16_t dither_lut[256] = {
      0x0000, 0xa011, 0xe033, 0x4022, 0x6077, 0xc066, 0x8044, 0x2055,
      0xc0ee, 0x60ff, 0x20dd, 0x80cc, 0xa099, 0x0088, 0x40aa, 0xe0bb,
      0x21cd, 0x81dc, 0xc1fe, 0x61ef, 0x41ba, 0xe1ab, 0xa189, 0x0198,
      0xe123, 0x4132, 0x0110, 0xa101, 0x8154, 0x2145, 0x6167, 0xc176,
      0x439a, 0xe38b, 0xa3a9, 0x03b8, 0x23ed, 0x83fc, 0xc3de, 0x63cf,
      0x8374, 0x2365, 0x6347, 0xc356, 0xe303, 0x4312, 0x0330, 0xa321,
      0x6257, 0xc246, 0x8264, 0x2275, 0x0220, 0xa231, 0xe213, 0x4202,
      0xa2b9, 0x02a8, 0x428a, 0xe29b, 0xc2ce, 0x62df, 0x22fd, 0x82ec,
      0x8734, 0x2725, 0x6707, 0xc716, 0xe743, 0x4752, 0x0770, 0xa761,
      0x47da, 0xe7cb, 0xa7e9, 0x07f8, 0x27ad, 0x87bc, 0xc79e, 0x678f,
      0xa6f9, 0x06e8, 0x46ca, 0xe6db, 0xc68e, 0x669f, 0x26bd, 0x86ac,
      0x6617, 0xc606, 0x8624, 0x2635, 0x0660, 0xa671, 0xe653, 0x4642,
      0xc4ae, 0x64bf, 0x249d, 0x848c, 0xa4d9, 0x04c8, 0x44ea, 0xe4fb,
      0x0440, 0xa451, 0xe473, 0x4462, 0x6437, 0xc426, 0x8404, 0x2415,
      0xe563, 0x4572, 0x0550, 0xa541, 0x8514, 0x2505, 0x6527, 0xc536,
      0x258d, 0x859c, 0xc5be, 0x65af, 0x45fa, 0xe5eb, 0xa5c9, 0x05d8,
      0xae79, 0x0e68, 0x4e4a, 0xee5b, 0xce0e, 0x6e1f, 0x2e3d, 0x8e2c,
      0x6e97, 0xce86, 0x8ea4, 0x2eb5, 0x0ee0, 0xaef1, 0xeed3, 0x4ec2,
      0x8fb4, 0x2fa5, 0x6f87, 0xcf96, 0xefc3, 0x4fd2, 0x0ff0, 0xafe1,
      0x4f5a, 0xef4b, 0xaf69, 0x0f78, 0x2f2d, 0x8f3c, 0xcf1e, 0x6f0f,
      0xede3, 0x4df2, 0x0dd0, 0xadc1, 0x8d94, 0x2d85, 0x6da7, 0xcdb6,
      0x2d0d, 0x8d1c, 0xcd3e, 0x6d2f, 0x4d7a, 0xed6b, 0xad49, 0x0d58,
      0xcc2e, 0x6c3f, 0x2c1d, 0x8c0c, 0xac59, 0x0c48, 0x4c6a, 0xec7b,
      0x0cc0, 0xacd1, 0xecf3, 0x4ce2, 0x6cb7, 0xcca6, 0x8c84, 0x2c95,
      0x294d, 0x895c, 0xc97e, 0x696f, 0x493a, 0xe92b, 0xa909, 0x0918,
      0xe9a3, 0x49b2, 0x0990, 0xa981, 0x89d4, 0x29c5, 0x69e7, 0xc9f6,
      0x0880, 0xa891, 0xe8b3, 0x48a2, 0x68f7, 0xc8e6, 0x88c4, 0x28d5,
      0xc86e, 0x687f, 0x285d, 0x884c, 0xa819, 0x0808, 0x482a, 0xe83b,
      0x6ad7, 0xcac6, 0x8ae4, 0x2af5, 0x0aa0, 0xaab1, 0xea93, 0x4a82,
      0xaa39, 0x0a28, 0x4a0a, 0xea1b, 0xca4e, 0x6a5f, 0x2a7d, 0x8a6c,
      0x4b1a, 0xeb0b, 0xab29, 0x0b38, 0x2b6d, 0x8b7c, 0xcb5e, 0x6b4f,
      0x8bf4, 0x2be5, 0x6bc7, 0xcbd6, 0xeb83, 0x4b92, 0x0bb0, 0xaba1
  };

  int16_t state = dither_lut[lfsr_state >> 8] ^ (lfsr_state << 8);
  lfsr_state = (uint16_t) state;
  return state;
}
