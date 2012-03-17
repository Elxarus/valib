#ifndef VALIB_AC3_HEADER_H
#define VALIB_AC3_HEADER_H

#include "../../parser.h"

class AC3FrameParser : public BasicFrameParser
{
public:
  static const SyncTrie sync_trie;

  AC3FrameParser() {}

  virtual bool      can_parse(int format) const { return format == FORMAT_AC3; }
  virtual SyncInfo  sync_info() const { return SyncInfo(sync_trie, 128, 3814); }

  // Frame header operations
  virtual size_t    header_size() const { return 8; }
  virtual bool      parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const;
  virtual bool      compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;

protected:
  virtual SyncInfo build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const;
};

#endif
