#ifndef VALIB_DTS_HEADER_H
#define VALIB_DTS_HEADER_H

#include "../../parser.h"

class DTSFrameParser : public BasicFrameParser
{
public:
  static const SyncTrie sync_trie;

  DTSFrameParser() {}

  virtual bool      can_parse(int format) const { return format == FORMAT_DTS; }
  virtual SyncInfo  sync_info() const { return SyncInfo(sync_trie, 96, 16384); }

  // Frame header operations
  virtual size_t    header_size() const { return 16; }
  virtual bool      parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const;
  virtual bool      compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;

  // Frame operations
  virtual bool      first_frame(const uint8_t *frame, size_t size);
  virtual bool      next_frame(const uint8_t *frame, size_t size);
  virtual void      reset();

protected:
  virtual SyncInfo build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const;
};

///////////////////////////////////////////////////////////////////////////////

class DTSHeader : public HeaderParser
{
public:
  DTSHeader() {};

  /////////////////////////////////////////////////////////
  // HeaderParser overrides

  virtual SyncTrie sync_trie() const;
  virtual size_t   header_size()    const  { return 16;    }
  virtual size_t   min_frame_size() const  { return 96;    }
  virtual size_t   max_frame_size() const  { return 16384; }
  virtual bool     can_parse(int format) const { return format == FORMAT_DTS; };

  virtual bool     parse_header(const uint8_t *hdr, HeaderInfo *h = 0) const;
  virtual bool     compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;
};

extern const DTSHeader dts_header;

#endif
