#ifndef VALIB_EAC3_HEADER_H
#define VALIB_EAC3_HEADER_H

#include "../../parser.h"

class EAC3FrameParser : public BasicFrameParser
{
public:
  static const SyncTrie sync_trie;

  EAC3FrameParser() {}

  virtual bool      can_parse(int format) const { return format == FORMAT_EAC3; }
  virtual SyncInfo  sync_info() const { return SyncInfo(sync_trie, 6, 4096); }

  // Frame header operations
  virtual size_t    header_size() const { return 6; }
  virtual bool      parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const;
  virtual bool      compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;

protected:
  virtual SyncInfo build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const;
};

#endif
