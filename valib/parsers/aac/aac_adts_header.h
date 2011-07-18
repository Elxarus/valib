#ifndef VALIB_AAC_ADTS_HEADER_H
#define VALIB_AAC_ADTS_HEADER_H

#include "../../parser.h"

class ADTSHeader : public HeaderParser
{
public:
  ADTSHeader() {};

  /////////////////////////////////////////////////////////
  // HeaderParser overrides

  virtual SyncTrie sync_trie() const;
  virtual size_t   header_size()    const  { return 8;    }
  virtual size_t   min_frame_size() const  { return 8;    }
  virtual size_t   max_frame_size() const  { return 8192; }
  virtual bool     can_parse(int format) const { return format == FORMAT_AAC_ADTS; }

  virtual bool     parse_header(const uint8_t *hdr, HeaderInfo *hinfo = 0) const;
  virtual bool     compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;
};

extern const ADTSHeader adts_header;

#endif
