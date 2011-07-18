#ifndef VALIB_EAC3_HEADER_H
#define VALIB_EAC3_HEADER_H

#include "../../parser.h"

class EAC3Header : public HeaderParser
{
public:
  EAC3Header() {};

  /////////////////////////////////////////////////////////
  // HeaderParser overrides

  virtual SyncTrie sync_trie() const;
  virtual size_t   header_size()    const  { return 6;    }
  virtual size_t   min_frame_size() const  { return 6;    }
  virtual size_t   max_frame_size() const  { return 4096; }
  virtual bool     can_parse(int format) const { return format == FORMAT_EAC3; };

  virtual bool     parse_header(const uint8_t *hdr, HeaderInfo *hinfo = 0) const;
  virtual bool     compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;
};

extern const EAC3Header eac3_header;

#endif
