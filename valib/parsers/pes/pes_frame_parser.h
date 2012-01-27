#ifndef PES_FRAME_PARSER_H
#define PES_FRAME_PARSER_H

#include "../../parser.h"

// Todo: min/max frame size???
// Currently min frame size = max header size.
//
// Frame parser only checks the stream number, not substream number because
// finding a substream number requires a header of size 268, that is larger
// than minimum packet size. This leads to loss of small packets.
//
// Substream is checked by PESParser.

class PESFrameParser : public BasicFrameParser
{
public:
  static const SyncTrie sync_trie;

  PESFrameParser() {}

  virtual bool      can_parse(int format) const { return format == FORMAT_PES; }
  virtual SyncInfo  sync_info() const { return SyncInfo(sync_trie, 6, 8192); }

  // Frame header operations
  virtual size_t    header_size() const { return 6; }
  virtual bool      parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const;
  virtual bool      compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;

protected:
  virtual SyncInfo build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const;
};

#endif
