#ifndef AC3_HEADER_H
#define AC3_HEADER_H

#include "parser.h"

class AC3Header : public HeaderParser
{
public:
  AC3Header() {};

  /////////////////////////////////////////////////////////
  // HeaderParser overrides
  // Max frame size for AC3 is 3814, but to support
  // AC3/SPDIF (padded format) we must specify SPDIF frame
  // size (1536 samples * 4 = 6144).

  virtual size_t   header_size()    const  { return 8;    }
  virtual size_t   min_frame_size() const  { return 128;  }
  virtual size_t   max_frame_size() const  { return 6144; }

  virtual bool     parse_header(const uint8_t *hdr, HeaderInfo *h = 0) const;
  virtual bool     compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;
};

extern const AC3Header ac3_header;

#endif
