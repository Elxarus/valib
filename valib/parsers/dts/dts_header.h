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
  bool master_audio;
  size_t dts_size;

  bool check_first_frame_size(const uint8_t *frame, size_t size);
  bool check_next_frame_size(const uint8_t *frame, size_t size);

  virtual SyncInfo build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const;
};

#endif
