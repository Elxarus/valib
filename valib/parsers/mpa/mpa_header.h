#ifndef VALIB_MPA_HEADER_H
#define VALIB_MPA_HEADER_H

#include "../../parser.h"

class MPAFrameParser : public BasicFrameParser
{
public:
  static const SyncTrie sync_trie;

  MPAFrameParser() {}

  virtual bool      can_parse(int format) const { return format == FORMAT_MPA; }
  virtual SyncInfo  sync_info() const { return SyncInfo(sync_trie, 32, 1728); }

  // Frame header operations
  virtual size_t    header_size() const { return 4; }
  virtual bool      parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const;
  virtual bool      compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;

protected:
  virtual SyncInfo build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const;
};

///////////////////////////////////////////////////////////////////////////////

class MPAHeader : public HeaderParser
{
public:
  MPAHeader() {};

  /////////////////////////////////////////////////////////
  // HeaderParser overrides

  virtual SyncTrie sync_trie() const;
  virtual size_t   header_size()    const  { return 4;    }
  virtual size_t   min_frame_size() const  { return 32;   }
  virtual size_t   max_frame_size() const  { return 1728; }
  virtual bool     can_parse(int format) const { return format == FORMAT_MPA; };

  virtual bool     parse_header(const uint8_t *hdr, HeaderInfo *hinfo = 0) const;
  virtual bool     compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;
};

extern const MPAHeader mpa_header;

#endif
