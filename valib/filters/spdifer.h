/*
  Encapsulates encoded stream into SPDIF according to IEC 61937

  Input:     SPDIF, AC3, MPA, DTS, Unknown
  Output:    SPDIF, DTS
  OFDD:      yes
  Buffering: block
  Timing:    first syncpoint
  Parameters:
    -
*/

#ifndef SPDIFER_H
#define SPDIFER_H

#include "sync.h"
#include "filter.h"
#include "data.h"
     

///////////////////////////////////////////////////////////////////////////////
// Spdifer class
///////////////////////////////////////////////////////////////////////////////

class Spdifer : public NullFilter
{
protected:
  enum { state_sync, state_spdif, state_drop_spdif, state_passthrough, state_drop_passthrough } state;

  Sync     sync_helper;
  DataBuf  frame_buf;    // frame & sync buffer
  size_t   frame_data;   // data size at frame buffer (excluding spdif header size)
  size_t   frames;       // number of frames sent
 
  Speakers sync_spk;     // stream format we're synced on

  // output
  Speakers out_spk;      // output format
  uint8_t *out_rawdata;  // output data
  size_t   out_size;     // output data size
  bool     out_flushing; // inter-stream flushing

  // stream info
  // filled by xxxx_syncinfo() functions
  Speakers stream_spk;   // stream format
  size_t frame_size;     // frame size of encoded stream (not spdif frame size)
  size_t nsamples;       // number of samples in frame
  int bs_type;           // bitstream type 32/16/14 bit big/little endian
  int magic;             // SPDIF stream identifier
  
  bool load_frame();
  void drop_frame();

  // fast inline sync detectors
  inline bool frame_sync(const uint8_t *buf) const;
  inline bool ac3_sync(const uint8_t *header) const;
  inline bool mpa_sync(const uint8_t *header) const;
  inline bool dts_sync(const uint8_t *header) const;

  // decode stream information
  bool frame_syncinfo(const uint8_t *buf);
  bool ac3_syncinfo(const uint8_t *buf);
  bool mpa_syncinfo(const uint8_t *buf);
  bool dts_syncinfo(const uint8_t *buf);

  union MPAHeader
  {
    MPAHeader() {};
    MPAHeader(uint32_t i) { raw = i; }

    uint32_t raw;
    struct
    {
      unsigned emphasis           : 2;
      unsigned original           : 1;
      unsigned copyright          : 1;
      unsigned mode_ext           : 2;
      unsigned mode               : 2;
      unsigned extension          : 1;
      unsigned padding            : 1;
      unsigned sampling_frequency : 2;
      unsigned bitrate_index      : 4;
      unsigned error_protection   : 1;
      unsigned layer              : 2;
      unsigned version            : 1;
      unsigned sync               : 12;
    };
  };

public:
  Spdifer();

  /////////////////////////////////////////////////////////
  // Spdifer interface

  void get_info(char *buf, size_t len);
  int  get_frames();

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual void reset();

  virtual bool query_input(Speakers spk) const;
  virtual bool process(const Chunk *chunk);

  virtual Speakers get_output() const;
  virtual bool is_empty() const;
  virtual bool get_chunk(Chunk *chunk);
};



///////////////////////////////////////////////////////////////////////////////
// Spdifer inlines
///////////////////////////////////////////////////////////////////////////////

inline bool Spdifer::frame_sync(const uint8_t *_buf) const
{
  return ac3_sync(_buf) || dts_sync(_buf) || mpa_sync(_buf);
}

inline bool Spdifer::ac3_sync(const uint8_t *_buf) const
{
  // 8 bit or 16 bit little endian steram sync
  if ((_buf[0] == 0x0b) && (_buf[1] == 0x77))
  {
    // constraints
    if (_buf[5] >= 0x60)         return false;   // 'bsid'
    if ((_buf[4] & 0x3f) > 0x25) return false;   // 'frmesizecod'
    if ((_buf[4] & 0xc0) > 0x80) return false;   // 'fscod'
    return true;
  }
  // 16 bit big endian steram sync
  else if ((_buf[1] == 0x0b) && (_buf[0] == 0x77))
  {
    // constraints
    if (_buf[4] >= 0x60)         return false;   // 'bsid'
    if ((_buf[5] & 0x3f) > 0x25) return false;   // 'frmesizecod'
    if ((_buf[5] & 0xc0) > 0x80) return false;   // 'fscod'
    return true;
  }
  else 
    return false;
}

inline bool Spdifer::mpa_sync(const uint8_t *_buf) const
{
  MPAHeader h = swab_u32(*(uint32_t *)_buf);

  if (h.sync != 0xfff)           return false;
  if (h.layer == 0)              return false;
  if (h.bitrate_index >= 15)     return false;
  if (h.sampling_frequency >= 3) return false;

  // for now we will not work with free-format
  if (h.bitrate_index == 0)      return false; 

  return true;
}

inline bool Spdifer::dts_sync(const uint8_t *_buf) const
{
  // 14 bits little endian bitstream
  if (_buf[0] == 0xff && _buf[1] == 0x1f &&
      _buf[2] == 0x00 && _buf[3] == 0xe8 &&
      (_buf[4] & 0xf0) == 0xf0 && _buf[5] == 0x07)
    return true;
  // 14 bits big endian bitstream
  else if (_buf[0] == 0x1f && _buf[1] == 0xff &&
           _buf[2] == 0xe8 && _buf[3] == 0x00 &&
           _buf[4] == 0x07 && (_buf[5] & 0xf0) == 0xf0)
    return true;
  // 16 bits little endian bitstream
  else if (_buf[0] == 0xfe && _buf[1] == 0x7f &&
           _buf[2] == 0x01 && _buf[3] == 0x80)
    return true;
  // 16 bits big endian bitstream
  else if (_buf[0] == 0x7f && _buf[1] == 0xfe &&
           _buf[2] == 0x80 && _buf[3] == 0x01)
    return true;
  else
  // no sync
    return false;
}

#endif