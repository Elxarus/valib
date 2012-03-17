#ifndef VALIB_AAC_ADTS_HEADER_H
#define VALIB_AAC_ADTS_HEADER_H

#include "../../parser.h"

class ADTSFrameParser : public BasicFrameParser
{
public:
  static const SyncTrie sync_trie;

  ADTSFrameParser() {}

  virtual bool      can_parse(int format) const { return format == FORMAT_AAC_ADTS; }
  virtual SyncInfo  sync_info() const { return SyncInfo(sync_trie, 8, 8192); }

  // Frame header operations
  virtual size_t    header_size() const { return 8; }
  virtual bool      parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const;
  virtual bool      compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;

protected:
  virtual SyncInfo build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const;
};

#endif
