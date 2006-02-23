#include <stdio.h>
#include <string.h>
#include "mpa_parser.h"
#include "mpa_tables.h"
#include "crc.h"


//////////////////////////////////////////////////////////////////////
// Support functions
//////////////////////////////////////////////////////////////////////
/*
inline uint16_t swabw(uint16_t w) { return ((w & 0xff) << 8) | (w >> 8); };
inline uint32_t swabd(uint32_t d) { return swabw(d >> 16) | (swabw((uint16_t)d) << 16); }
#define min(a, b) ((a) < (b)? (a): (b))
*/
#define CRC16_POLYNOMIAL 0x8005

unsigned short calc_crc(unsigned short crc, unsigned short *data, int len)
// len  - data length in bits (sic!)
{
  while (len >= 16)
  {
    crc = swab_u16(*data) ^ crc;
    data++;

    #define calc_crc_step(crc) \
      crc = (crc << 1) ^ (CRC16_POLYNOMIAL >> ((~crc & 0x8000) >> 11))

//  calc_crc_step(crc) is equivalent to:
//  {
//    if (crc & 0x8000)
//      (crc <<= 1) ^= CRC16_POLYNOMIAL;
//    else
//      crc <<= 1;
//  }

    calc_crc_step(crc); calc_crc_step(crc); calc_crc_step(crc); calc_crc_step(crc);
    calc_crc_step(crc); calc_crc_step(crc); calc_crc_step(crc); calc_crc_step(crc);
    calc_crc_step(crc); calc_crc_step(crc); calc_crc_step(crc); calc_crc_step(crc);
    calc_crc_step(crc); calc_crc_step(crc); calc_crc_step(crc); calc_crc_step(crc);

    len -= 16;
  }

  if (len)
  {
    crc = (swab_u16(*(data++)) >> (16 - len) << (16 - len)) ^ crc;
    while (len--) calc_crc_step(crc);
  }

  return crc;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


MPAParser::MPAParser()
{
  samples.allocate(2, MPA_NSAMPLES);
  frame.allocate(MPA_MAX_FRAME_SIZE);

  synth[0] = new SynthBufferFPU();
  synth[1] = new SynthBufferFPU();

  // setup syncronization scanner
  scanner.set_standard(SYNCMASK_MPA);

  // always useful
  reset();
}


MPAParser::~MPAParser()
{
  if (synth[0]) delete synth[0];
  if (synth[1]) delete synth[1];
}

///////////////////////////////////////////////////////////////////////////////
// Parser overrides

void 
MPAParser::reset()
{
  BaseParser::reset();

  frame.zero();
  samples.zero();

  if (synth[0]) synth[0]->reset();
  if (synth[1]) synth[1]->reset();
}

void 
MPAParser::get_info(char *buf, unsigned len) const 
{
  char info[1024];
  sprintf(info, 
    "MPEG Audio\n"
    "speakers: %s\n"
    "ver: %s\n"
    "frame size: %i bytes\n"
    "stream: %s\n"
    "bitrate: %ikbps\n"
    "sample rate: %iHz\n"
    "bandwidth: %ikHz/%ikHz\n\0",
    spk.mode_text(),
    bsi.ver? "MPEG2 LSF": "MPEG1", 
    bsi.frame_size,
    (bs_type == BITSTREAM_8? "8 bit": "16bit big endian"), 
    bsi.bitrate / 1000,
    bsi.freq,
    bsi.jsbound * bsi.freq / SBLIMIT / 1000 / 2,
    bsi.sblimit * bsi.freq / SBLIMIT / 1000 / 2);
  memcpy(buf, info, MIN(len, strlen(info)+1));
}

bool 
MPAParser::decode_frame()
{
  ///////////////////////////////////////////////////////
  // Decode frame

  bool ok = false;

  bitstream.set_ptr(frame + 4 + (hdr.error_protection << 1), bs_type);
  switch (bsi.layer)
  {
    case MPA_LAYER_I:  ok = I_decode_frame();  break;
    case MPA_LAYER_II: ok = II_decode_frame(); break;
  }

  ///////////////////////////////////////////////////////
  // Handle errors
  // todo: return previous frame data?

  if (!ok) errors++;

  return ok;
}

///////////////////////////////////////////////////////////////////////////////
// BaseParser overrides

size_t 
MPAParser::header_size() const
{ 
  return 4;
}

bool 
MPAParser::load_header(uint8_t *_buf)
{
  Header h;

  // MPA low and big endians have ambigous headers
  // so first we check low endian as most used and only
  // then try big endian

  // 8 bit or 16 bit little endian steram sync
  if ((_buf[0] == 0xff)         && // sync
     ((_buf[1] & 0xf0) == 0xf0) && // sync
     ((_buf[1] & 0x06) != 0x00) && // layer
     ((_buf[2] & 0xf0) != 0xf0) && // bitrate
     ((_buf[2] & 0xf0) != 0x00) && // prohibit free-format
     ((_buf[2] & 0x0c) != 0x0c))   // sample rate
  {
    uint32_t header = *(uint32_t *)_buf;
    h = swab_u32(header);
    bs_type = BITSTREAM_8;
  }
  else
  // 16 bit big endian steram sync
  if ((_buf[1] == 0xff)         && // sync
     ((_buf[0] & 0xf0) == 0xf0) && // sync
     ((_buf[0] & 0x06) != 0x00) && // layer
     ((_buf[3] & 0xf0) != 0xf0) && // bitrate
     ((_buf[3] & 0xf0) != 0x00) && // prohibit free-format
     ((_buf[3] & 0x0c) != 0x0c))   // sample rate
  {
    uint32_t header = *(uint32_t *)_buf;
    h = (header >> 16) | (header << 16);
    bs_type = BITSTREAM_16BE;
  }
  else
    return false;

  // common information
  int ver = 1 - h.version;
  int layer = 3 - h.layer;
  int bitrate = bitrate_tbl[ver][layer][h.bitrate_index] * 1000;
  int sample_rate = freq_tbl[ver][h.sampling_frequency];

  // frame size calculation
  frame_size = bitrate * slots_tbl[layer] / sample_rate + h.padding;
  if (layer == 0) // MPA_LAYER_I
    frame_size *= 4;

  nsamples = layer == 0? 384: 1152;
  spk = Speakers(FORMAT_MPA, (h.mode == 3)? MODE_MONO: MODE_STEREO, sample_rate);
  return true;
}

bool
MPAParser::prepare()
{
  return decode_header();
}


//////////////////////////////////////////////////////////////////////
// MPA parsing
//////////////////////////////////////////////////////////////////////

bool 
MPAParser::decode_header()
{
  if (bs_type == BITSTREAM_8)
  {
    uint32_t header = *(uint32_t *)frame.get_data();
    hdr = swab_u32(header);
  }
  else
  {
    uint32_t header = *(uint32_t *)frame.get_data();
    hdr = (header >> 16) | (header << 16);
  }

  hdr.error_protection = ~hdr.error_protection;

  // integrity check
  if (hdr.layer == 0)              return false;
  if (hdr.bitrate_index >= 15)     return false;
  if (hdr.sampling_frequency >= 3) return false;

  // for now we will not work with free-format
  if (hdr.bitrate_index == 0)      return false;  

  // common information
  bsi.ver       = 1 - hdr.version;
  bsi.mode      = hdr.mode;
  bsi.layer     = 3 - hdr.layer;
  bsi.bitrate   = bitrate_tbl[bsi.ver][bsi.layer][hdr.bitrate_index] * 1000;
  bsi.freq      = freq_tbl[bsi.ver][hdr.sampling_frequency];
  bsi.nch       = bsi.mode == MPA_MODE_SINGLE? 1: 2;
  bsi.nsamples  = bsi.layer == MPA_LAYER_I? SCALE_BLOCK * SBLIMIT: SCALE_BLOCK * SBLIMIT * 3;

  // frame size calculation
  bsi.frame_size = bsi.bitrate * slots_tbl[bsi.layer] / bsi.freq + hdr.padding;
  if (bsi.layer == MPA_LAYER_I) 
    bsi.frame_size *= 4;

  // layerII: table select
  II_table = 0;
  if (bsi.layer == MPA_LAYER_II)
  {
    // todo: check for allowed bitrate ??? (look at sec 2.4.2.3 of ISO 11172-3)
    if (bsi.ver)
      // MPEG2 LSF
      II_table = 4; 
    else
    {
      // MPEG1
      int bitrate_per_ch = bsi.mode == MPA_MODE_SINGLE? hdr.bitrate_index: II_half_bitrate_tbl[hdr.bitrate_index];
      II_table = II_table_tbl[hdr.sampling_frequency][bitrate_per_ch]; 
    }
  }

  // subband information
  bsi.sblimit = bsi.layer == MPA_LAYER_II?
                  II_sblimit_tbl[II_table]:
                  SBLIMIT;

  bsi.jsbound = bsi.mode == MPA_MODE_JOINT? 
                  jsbound_tbl[bsi.layer][hdr.mode_ext]: 
                  bsi.sblimit;

  return true; 
}


///////////////////////////////////////////////////////////////////////////////
//  Layer II
///////////////////////////////////////////////////////////////////////////////


bool 
MPAParser::II_decode_frame()
{
  int sb, ch;
  int nch     = bsi.nch;
  int sblimit = bsi.sblimit;
  int jsbound = bsi.jsbound;
  int table   = II_table;

  int16_t  bit_alloc[MPA_NCH][SBLIMIT]; 
  sample_t scale[MPA_NCH][3][SBLIMIT];
  
  /////////////////////////////////////////////////////////
  // Load bitalloc
 
  const int16_t *ba_bits = II_ba_bits_tbl[table];
  int crc_bits = 0;

  if (nch == 1)
  {
    for (sb = 0; sb < sblimit; sb++)
    {
      int bits = ba_bits[sb];
      crc_bits += bits;
      bit_alloc[0][sb] = II_ba_tbl[table][sb][bitstream.get(bits)];
    }
    
    for (sb = sblimit; sb < SBLIMIT; sb++) 
      bit_alloc[0][sb] = 0;
  }
  else
  {
    for (sb = 0; sb < jsbound; sb++) 
    {
      int bits = ba_bits[sb];
      crc_bits += bits << 1;
      if (bits)
      {
        bit_alloc[0][sb] = II_ba_tbl[table][sb][bitstream.get(bits)];
        bit_alloc[1][sb] = II_ba_tbl[table][sb][bitstream.get(bits)];
      }
      else
      {
        bit_alloc[0][sb] = II_ba_tbl[table][sb][0];
        bit_alloc[1][sb] = II_ba_tbl[table][sb][0];
      }
    }

    for (sb = jsbound; sb < sblimit; sb++)
    {
      int bits = ba_bits[sb];
      crc_bits += bits;
      if (bits)
        bit_alloc[0][sb] = bit_alloc[1][sb] = II_ba_tbl[table][sb][bitstream.get(bits)];
      else
        bit_alloc[0][sb] = bit_alloc[1][sb] = II_ba_tbl[table][sb][0];
    }
    
    for (sb = sblimit; sb < SBLIMIT; sb++) 
      bit_alloc[0][sb] = bit_alloc[1][sb] = 0;
  }

  /////////////////////////////////////////////////////////
  // Load scalefactors bitalloc
  
  uint16_t scfsi[2][SBLIMIT];
  for (sb = 0; sb < sblimit; sb++) 
    for (ch = 0; ch < nch; ch++)    // 2 bit scfsi 
      if (bit_alloc[ch][sb]) 
      {
        crc_bits += 2;
        scfsi[ch][sb] = (uint16_t) bitstream.get(2);
      }

  // do we need this?
  for (sb = sblimit; sb < SBLIMIT; sb++) 
    for (ch = 0; ch < nch; ch++)   
      scfsi[ch][sb] = 0;

  /////////////////////////////////////////////////////////
  // CRC check
  // todo: big endian CRC check

  if (hdr.error_protection && bs_type == BITSTREAM_8)
  {
    uint16_t crc = 0xffff;
    crc = calc_crc(crc, (uint16_t*)(frame+2), 16);
    crc = calc_crc(crc, (uint16_t*)(frame+6), crc_bits);
    uint16_t crc_test = swab_u16(*(uint16_t*)(frame+4));
    if (crc != crc_test)
      return false;
  }
        
  /////////////////////////////////////////////////////////
  // Load scalefactors

  sample_t c;
  for (sb = 0; sb < sblimit; sb++) 
    for (ch = 0; ch < nch; ch ++) 
    {
      int ba = bit_alloc[ch][sb];
      if (ba)
      {
        if (ba > 0)
          c = c_tbl[ba];
        else 
          switch (ba)
          {
          case -5:  c = c_tbl[0]; break;
          case -7:  c = c_tbl[1]; break;
          case -10: c = c_tbl[2]; break;
          }

        switch (scfsi[ch][sb]) 
        {
          case 0 :  // all three scale factors transmitted 
            scale[ch][0][sb] = scale_tbl[bitstream.get(6)] * c;
            scale[ch][1][sb] = scale_tbl[bitstream.get(6)] * c;
            scale[ch][2][sb] = scale_tbl[bitstream.get(6)] * c;
            break;
          
          case 1 :  // scale factor 1 & 3 transmitted 
            scale[ch][0][sb] =
            scale[ch][1][sb] = scale_tbl[bitstream.get(6)] * c;
            scale[ch][2][sb] = scale_tbl[bitstream.get(6)] * c;
            break;
          
          case 3 :  // scale factor 1 & 2 transmitted
            scale[ch][0][sb] = scale_tbl[bitstream.get(6)] * c;
            scale[ch][1][sb] =
            scale[ch][2][sb] = scale_tbl[bitstream.get(6)] * c;
            break;
          
          case 2 :    // only one scale factor transmitted
            scale[ch][0][sb] =
            scale[ch][1][sb] =
            scale[ch][2][sb] = scale_tbl[bitstream.get(6)] * c;
            break;
          
          default : break;      
        }
      }
      else 
      {
        scale[ch][0][sb] = scale[ch][1][sb] =
          scale[ch][2][sb] = 0;         
      }         
    }

  // do we need this?
  for (sb = sblimit; sb < SBLIMIT; sb++) 
    for (ch = 0; ch < nch; ch++) 
      scale[ch][0][sb] = scale[ch][1][sb] =
        scale[ch][2][sb] = 0;

  /////////////////////////////////////////////////////////
  // Decode fraction and synthesis

  int clip = 0;  
  sample_t *sptr[MPA_NCH];

  for (int i = 0; i < SCALE_BLOCK; i++) 
  {
    sptr[0] = &samples[0][i * SBLIMIT * 3];
    sptr[1] = &samples[1][i * SBLIMIT * 3];
    II_decode_fraction(sptr, bit_alloc, scale, i >> 2);
    for (ch = 0; ch < nch; ch++)
    {
      synth[ch]->synth(&samples[ch][i * SBLIMIT * 3              ]);
      synth[ch]->synth(&samples[ch][i * SBLIMIT * 3 + 1 * SBLIMIT]);
      synth[ch]->synth(&samples[ch][i * SBLIMIT * 3 + 2 * SBLIMIT]);
    }
  }

  return true;
}


void 
MPAParser::II_decode_fraction(
  sample_t *fraction[MPA_NCH],
  int16_t  bit_alloc[MPA_NCH][SBLIMIT],
  sample_t scale[MPA_NCH][3][SBLIMIT],
  int x)
{
  int sb, ch;
  int nch     = bsi.nch;
  int sblimit = bsi.sblimit;
  int jsbound = bsi.jsbound;

  uint16_t s0, s1, s2;
  int16_t d;
  int16_t ba; // signed!

  for (sb = 0; sb < sblimit; sb++) 
    for (ch = 0; ch < ((sb < jsbound)? nch: 1 ); ch++) 
    {
      // ba means number of bits to read;
      // negative numbers mean sample triplets
      ba = bit_alloc[ch][sb];

      if (ba) 
      {
        if (ba > 0)
        {                                        
          d  = d_tbl[ba]; // ba > 0 => ba = quant
          s0 = (uint16_t) bitstream.get(ba);
          s1 = (uint16_t) bitstream.get(ba);
          s2 = (uint16_t) bitstream.get(ba);

          ba = 16 - ba;  // number of bits we should shift
        }
        else // nlevels = 3, 5, 9; ba = -5, -7, -10
        {  
          // packed triplet of samples
          ba = -ba;
          unsigned int pack = (unsigned int) bitstream.get(ba);
          switch (ba)
          {
          case 5:
            s0 = pack % 3; pack /= 3;
            s1 = pack % 3; pack /= 3;
            s2 = pack % 3;
            d  = d_tbl[0];
            ba = 14;
            break;

          case 7:
            s0 = pack % 5; pack /= 5;
            s1 = pack % 5; pack /= 5;
            s2 = pack % 5;
            d  = d_tbl[1];
            ba = 13;
            break;

          case 10:
            s0 = pack % 9; pack /= 9;
            s1 = pack % 9; pack /= 9;
            s2 = pack % 9;
            d  = d_tbl[2];
            ba = 12;
            break;
          } 
        } // if (ba > 0) .. else ..

        #define dequantize(r, s, d, bits)                     \
        {                                                     \
          s = ((unsigned short) s) << bits;                   \
          s = (s & 0x7fff) | (~s & 0x8000);                   \
          r = (sample_t)((short)(s) + d) * scale[ch][x][sb];  \
        }

        #define dequantize2(r1, r2, s, d, bits)               \
        {                                                     \
          s  = ((unsigned short) s) << bits;                  \
          s  = (s & 0x7fff) | (~s & 0x8000);                  \
          sample_t f = sample_t((short)(s) + d);              \
          r1 = f * scale[0][x][sb];                           \
          r2 = f * scale[1][x][sb];                           \
        }

        if (nch > 1 && sb >= jsbound)
        {
          dequantize2(fraction[0][sb              ], fraction[1][sb              ], s0, d, ba);
          dequantize2(fraction[0][sb + 1 * SBLIMIT], fraction[1][sb + 1 * SBLIMIT], s1, d, ba);
          dequantize2(fraction[0][sb + 2 * SBLIMIT], fraction[1][sb + 2 * SBLIMIT], s2, d, ba);
        }
        else
        {
          dequantize(fraction[ch][sb              ], s0, d, ba);
          dequantize(fraction[ch][sb + 1 * SBLIMIT], s1, d, ba);
          dequantize(fraction[ch][sb + 2 * SBLIMIT], s2, d, ba);
        }
      }
      else // ba = 0; no sample transmitted 
      {         
        fraction[ch][sb              ] = 0.0;
        fraction[ch][sb + 1 * SBLIMIT] = 0.0;
        fraction[ch][sb + 2 * SBLIMIT] = 0.0;
        if (nch > 1 && sb >= jsbound)
        {
          fraction[1][sb              ] = 0.0;
          fraction[1][sb + 1 * SBLIMIT] = 0.0;
          fraction[1][sb + 2 * SBLIMIT] = 0.0;
        }
      } // if (ba) ... else ...
    } // for (ch = 0; ch < ((sb < jsbound)? nch: 1 ); ch++)
  // for (sb = 0; sb < sblimit; sb++)
    
  for (ch = 0; ch < nch; ch++) 
    for (sb = sblimit; sb < SBLIMIT; sb++) 
    {
      fraction[ch][sb              ] = 0.0;
      fraction[ch][sb +     SBLIMIT] = 0.0;
      fraction[ch][sb + 2 * SBLIMIT] = 0.0;
    }
}


///////////////////////////////////////////////////////////////////////////////
//  Layer I
///////////////////////////////////////////////////////////////////////////////


bool 
MPAParser::I_decode_frame()
{
  int ch, sb;
  int nch     = bsi.nch;
  int sblimit = bsi.sblimit;
  int jsbound = bsi.jsbound;
  
  int16_t  bit_alloc[MPA_NCH][SBLIMIT]; 
  sample_t scale[MPA_NCH][SBLIMIT];
  
  /////////////////////////////////////////////////////////
  // CRC check

  if (hdr.error_protection)
  {
    uint16_t crc = 0xffff;
    uint16_t crc_bits = jsbound;
    crc_bits = (crc_bits << 3) + ((32 - crc_bits) << 2);

    crc = calc_crc(crc, (uint16_t*)(frame+2), 16);
    crc = calc_crc(crc, (uint16_t*)(frame+6), crc_bits);
    uint16_t crc_test = swab_u16(*(uint16_t*)(frame+4));
    if (crc != crc_test)
      return false;
  }
        
  /////////////////////////////////////////////////////////
  // Load bitalloc
  
  for (sb = 0; sb < jsbound; sb++) 
    for (ch = 0; ch < nch; ch++)
      bit_alloc[ch][sb] = bitstream.get(4);
    
  for (sb = jsbound; sb < SBLIMIT; sb++) 
    bit_alloc[0][sb] = bit_alloc[1][sb] = bitstream.get(4);
    
  /////////////////////////////////////////////////////////
  // Load scale

  for (sb = 0; sb < SBLIMIT; sb++) 
    for (ch = 0; ch < nch; ch++)
      if (bit_alloc[ch][sb])
        scale[ch][sb] = scale_tbl[bitstream.get(6)]; // 6 bit per scale factor
      else                    
        scale[ch][sb] = 0;
      
  /////////////////////////////////////////////////////////
  // Decode fraction and synthesis

  int clip = 0;
  sample_t *sptr[2];
  for (int i = 0; i < SCALE_BLOCK * SBLIMIT; i += SBLIMIT) 
  {
    sptr[0] = &samples[0][i];
    sptr[1] = &samples[1][i];
    I_decode_fraction(sptr, bit_alloc, scale);
    for (ch = 0; ch < nch; ch++)
      synth[ch]->synth(&samples[ch][i]);
  }

  return true;
}



void 
MPAParser::I_decode_fraction(
  sample_t *fraction[MPA_NCH],
  int16_t  bit_alloc[MPA_NCH][SBLIMIT],
  sample_t scale[MPA_NCH][SBLIMIT])
{
  int sb, ch;
  int nch     = bsi.nch;
  int sblimit = bsi.sblimit;
  int jsbound = bsi.jsbound;

  int sample[MPA_NCH][SBLIMIT];
  int ba;

  /////////////////////////////////////////////////////////
  // buffer samples

  for (sb = 0; sb < jsbound; sb++) 
    for (ch = 0; ch < nch; ch++)
    {
      ba = bit_alloc[ch][sb];
      if (ba)
        sample[ch][sb] = (unsigned int) bitstream.get(ba + 1);
      else 
        sample[ch][sb] = 0;
    }  

  for (sb = jsbound; sb < SBLIMIT; sb++) 
  {
    ba = bit_alloc[0][sb];
    int s;
    if (ba)
      s = (unsigned int) bitstream.get(ba + 1);
    else 
      s = 0;

    for (ch = 0; ch < nch; ch++)
      sample[ch][sb] = s;
  }      
  
  /////////////////////////////////////////////////////////
  // Dequantize
  
  for (sb = 0; sb < SBLIMIT; sb++)
    for (ch = 0; ch < nch; ch++)
      if (bit_alloc[ch][sb]) 
      {
        ba = bit_alloc[ch][sb] + 1;
        
        if (((sample[ch][sb] >> (ba-1)) & 1) == 1)
          fraction[ch][sb] = 0.0;
        else 
          fraction[ch][sb] = -1.0;

        fraction[ch][sb] += (sample_t) (sample[ch][sb] & ((1<<(ba-1))-1)) /
          (sample_t) (1L<<(ba-1));
        
        fraction[ch][sb] =
          (sample_t) (fraction[ch][sb]+1.0/(sample_t)(1L<<(ba-1))) *
          (sample_t) (1L<<ba) / (sample_t) ((1L<<ba)-1);
      }
      else 
        fraction[ch][sb] = 0.0;
      
  /////////////////////////////////////////////////////////
  // Denormalize
      
  for (ch = 0; ch < nch; ch++)
    for (sb = 0; sb < SBLIMIT; sb++) 
      fraction[ch][sb] *= scale[ch][sb];
}
