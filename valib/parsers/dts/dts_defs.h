/*
  DTS parser definitions
*/

#ifndef VALIB_DTS_DEFS_H
#define VALIB_DTS_DEFS_H

struct huff_entry_t
{
  int length;
  int code;
  int value;
};


#define DTS_NCHANNELS      6
#define DTS_MAX_SAMPLES    4096
#define DTS_MAX_FRAME_SIZE 16384

#define DTS_SUBFRAMES_MAX     16
#define DTS_PRIM_CHANNELS_MAX 5
#define DTS_SUBBANDS          32
#define DTS_ABITS_MAX         32 // Should be 28
#define DTS_SUBSUBFAMES_MAX   4
#define DTS_LFE_MAX           3

#endif
