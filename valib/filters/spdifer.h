/*
  Encapsulates encoded stream into SPDIF according to IEC 61937

  Speakers: can change format
  Input formats:  SPDIF, AC3, MPA, DTS, Unknown
  Output formats: SPDIF, DTS
  Format conversions:
    SPDIF   -> SPDIF
    AC3     -> SPDIF
    MPA     -> SPDIF
    DTS     -> SPDIF
    DTS     -> DTS    (only if DTS bitrate is too high; this allows to decode DTS instead of SPDIF-passthrough)
    Unknown -> SPDIF  (only if source stream contains AC3, MPA or DTS stream)
    Unknown -> DTS    (only if source stream contains DTS stream and DTS bitrate is too high)
  Buffering: no
  Timing: apply input timestamp to the first syncpoint found at the input data
  Parameters:
    -
*/

#ifndef SPDIFER_H
#define SPDIFER_H

#include "sync.h"
#include "filter.h"

class Spdifer : public NullFilter
{
protected:
  DataBuf frame_buf;
  Sync    sync_helper;

  int bs_type;         // bitstream type 32/16/14 bit big/little endian
  int frame_size;      // frame size of encoded stream (not spdif frame size)
  int frame_data;      // data size in frame buffer (excluding spdif header size)
  enum { state_sync, state_drop, state_spdif, state_passthrough } state;

//  int sync_count;      // number of packets to sync
  Speakers stream_spk; // Stream speakers config
  int magic;           // SPDIF stream identifier
  int nsamples;        // number of samples in frame
  int frames;          // number of frames
  
  // fast inline sync detectors
  inline bool sync(const uint8_t *buf) const;
  inline bool ac3_sync(const uint8_t *header) const;
  inline bool mpa_sync(const uint8_t *header) const;
  inline bool dts_sync(const uint8_t *header) const;

  // decode stream information
  bool syncinfo(const uint8_t *buf);
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

  void get_info(char *buf, int len);
  int  get_frames();

  // Filter interface
  virtual void reset();
  virtual bool query_input(Speakers spk) const;
  virtual bool set_input(Speakers spk);
  virtual bool process(const Chunk *chunk);
  virtual bool get_chunk(Chunk *chunk);
  virtual Speakers get_output();
};

inline bool Spdifer::sync(const uint8_t *_buf) const
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
  MPAHeader h = swab32(*(uint32_t *)_buf);

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