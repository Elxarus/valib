#ifndef AC3_PARSER_H
#define AC3_PARSER_H

#include "parser.h"
#include "bitstream.h"

#include "ac3_defs.h"
#include "ac3_imdct.h"

///////////////////////////////////////////////////////////////////////////////
// Defines:
// AC3_DEBUG          - allows access to all private class data
// AC3_DEBUG_NOIMDCT  - disable IMDCT (samples will return coeffitients)
// AC3_DEBUG_NODITHER - disable dithering
///////////////////////////////////////////////////////////////////////////////

// todo: decode_block() for per-block decode

class AC3Info
{
public:
  int       bitrate;     // bitrate     (derived from frame_size & halfrate)
  int       halfrate;    // halfrate    (derived from bsid)

  int       fscod;       // sample rate code
  int       frmsizecod;  // frame size code

  int       bsid;        // bitstream identification
  int       bsmod;       // bitestream mode

  int       acmod;       // audio coding mode
  int       dsurmod;     // dolby surround mode
  bool      lfeon;       // lfe channel present

  sample_t  clev;        // center mix level
  sample_t  slev;        // surround mix level

  bool      compre;      // compression gain word exists
  sample_t  compr;       // compression gain word
  bool      compr2e;     // compression gain word 2 exists
  sample_t  compr2;      // compression gain word 2

  int       dialnorm;    // dialog normalization 
  int       dialnorm2;   // dialog normalization 2

  bool      langcode;    // language code exists
  int       langcod;     // language code
  bool      langcod2e;   // language code 2 exists
  int       langcod2;    // language code 2

  bool      audprodie;   // audio production information exists
  int       mixlevel;    // mixing level (SPL)
  int       roomtyp;     // room type
                        
  bool      audprodi2e;  // audio production information 2 exists
  int       mixlevel2;   // mixing level (SPL) 2
  int       roomtyp2;    // room type 2

  bool      copyrightb;  // copyright bit
  bool      origbs;      // original bitstream

  struct {
    int hours;
    int mins;
    int secs;
    int frames;
    int fracs;
  } timecode;            // timecode
};

class AC3FrameState
{
public:
  // general 
  bool blksw[5];         // non-reusable between blocks
  bool dithflag[5];      // non-reusable between blocks

  sample_t dynrng;       // needs reset at new frame
  sample_t dynrng2;      // needs reset at new frame

  // coupling info

  bool cplinu;
  bool chincpl[5];
  bool phsflginu;
  int  cplstrtmant;
  int  cplendmant;
  int  ncplbnd;
  int  cplbnd[18];
  sample_t cplco[5][18];

  // channel info

  int rematflg;
  int endmant[5];

  // exponents

  int8_t cplexps[256];
  int8_t exps[5][256];
  int8_t gainrng[5];     // non-reusable between blocks
  int8_t lfeexps[7];

  // bit allocation info

  int sdecay;
  int fdecay;
  int sgain;
  int dbknee;
  int floor;

  int cplsnroffset;
  int snroffset[5];
  int lfesnroffset;

  int cplfgain;
  int fgain[5];
  int lfefgain;

  int cplfleak;
  int cplsleak;

  // delta bit allocation

  int  cpldeltbae;       // needs reset at new frame
  int  deltbae[5];       // needs reset at new frame

  int8_t cpldeltba[50];
  int8_t deltba[5][50];

  // bit allocation

  int8_t cplbap[256];
  int8_t bap[5][256];
  int8_t lfebap[7];
};

class AC3Parser : public BaseParser, public AC3Info, public AC3FrameState
{
public:
  AC3Parser();
  ~AC3Parser();

  /////////////////////////////////////////////////////////
  // Parser overrides

  void reset();
  bool decode_frame();
  void get_info(char *buf, unsigned len) const;

protected:

  /////////////////////////////////////////////////////////
  // BaseParser overrides

  unsigned sync(uint8_t **buf, uint8_t *end);
  bool check_crc();
  bool start_decode();

#ifndef AC3_DEBUG 
protected:
#else
public:
#endif

  /////////////////////////////////////////////////////////
  // AC3 parse

  SampleBuf delay;     // delayed samples buffer
  IMDCT     imdct;     // IMDCT
  ReadBS    bs;        // Bitstream

  int block;

  inline unsigned ac3_sync(uint8_t *buf) const;
  bool decode_block();
  bool parse_header();
  bool parse_block();
  bool parse_exponents(int8_t *exps, int8_t absexp, int expstr, int nexpgrps);
  bool parse_deltba(int8_t *deltba);
  void parse_coeff(samples_t samples);

};


inline unsigned
AC3Parser::ac3_sync(uint8_t *_buf) const
{
  static const int halfrate_tbl[12] = 
  { 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3
  };
  static const int bitrate_tbl[] = 
  { 
    32,  40,  48,  56,  64,  80,  96, 112,
   128, 160, 192, 224, 256, 320, 384, 448,
   512, 576, 640 
  };

  /////////////////////////////////////////////////////////
  // 8 bit or 16 bit little endian steram sync
  if ((_buf[0] == 0x0b) && (_buf[1] == 0x77))
  {
    if (_buf[5] >= 0x60)         // 'bsid'
      return 0;

    if ((_buf[4] & 0x3f) > 0x25) // 'frmesizecod'
      return 0;

    if ((_buf[4] & 0xc0) > 0x80) // 'fscod'
      return 0;

    int halfrate = halfrate_tbl[_buf[5] >> 3];
    int frmsizecod = (_buf[4] & 0x3f);
    int bitrate = bitrate_tbl[frmsizecod >> 1];

    switch (_buf[4] & 0xc0) 
    {
      case 0:    return 4 * bitrate;
      case 0x40: return 2 * (320 * bitrate / 147 + (frmsizecod & 1));
      case 0x80: return 6 * bitrate;
      default:   return 0;
    }
  }
  /////////////////////////////////////////////////////////
  // 16 bit big endian steram sync
  else if ((_buf[1] == 0x0b) && (_buf[0] == 0x77))
  {
    if (_buf[4] >= 0x60)         // 'bsid'
      return 0;

    if ((_buf[5] & 0x3f) > 0x25) // 'frmesizecod'
      return 0;

    if ((_buf[5] & 0xc0) > 0x80) // 'fscod'
      return 0;

    int halfrate = halfrate_tbl[_buf[4] >> 3];
    int frmsizecod = (_buf[5] & 0x3f);
    int bitrate = bitrate_tbl[frmsizecod >> 1];

    switch (_buf[5] & 0xc0) 
    {
      case 0:    return 4 * bitrate;
      case 0x40: return 2 * (320 * bitrate / 147 + (frmsizecod & 1));
      case 0x80: return 6 * bitrate;
      default:   return 0;
   }
  }
  else
    return 0;
}

#endif