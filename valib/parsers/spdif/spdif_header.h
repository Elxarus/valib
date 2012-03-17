#ifndef VALIB_SPDIF_HEADER_H
#define VALIB_SPDIF_HEADER_H

#include "../../parser.h"
#include "spdifable_header.h"

/////////////////////////////////////////////////////////
// Max frame size for SPDIF 8192
// Header size, min and max frame sizes are determined
// by the number of samples the frame contains:
//
//       header   min    max
// ------------------------
// MPA:       4   384    1152
// AC3:       8  1536    1536
// DTS:      16   192    4096 (only 2048 is supported by SPDIF)
//
// Header size = SPDIF header size + max header size = 32
// Minimum SPDIF frame size = 192 * 4 = 768
// Maximum SPDIF frame size = 2048 * 4 = 8192

class SPDIFFrameParser : public BasicFrameParser
{
public:
  static const SyncTrie sync_trie;

  SPDIFFrameParser() {}

  virtual bool      can_parse(int format) const { return format == FORMAT_SPDIF; }
  virtual SyncInfo  sync_info() const { return SyncInfo(sync_trie, 768, 8192); }

  // Frame header operations
  virtual size_t    header_size() const { return 32; }
  virtual bool      parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const;
  virtual bool      compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;

protected:
  SpdifableFrameParser spdifable_parser;

  struct spdif_header_s
  {
    uint16_t zero1;
    uint16_t zero2;
    uint16_t zero3;
    uint16_t zero4;

    uint16_t sync1;   // Pa sync word 1
    uint16_t sync2;   // Pb sync word 2
    uint16_t type;    // Pc data type
    uint16_t len;     // Pd length-code (bits)
  };
};
#endif
