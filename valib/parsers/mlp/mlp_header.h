#ifndef VALIB_MLP_HEADER_H
#define VALIB_MLP_HEADER_H

#include "../../parser.h"

// One frame for MLP is a distance between 2 major sync headers.
// One frame consists of a sequence of subframes, containing 40, 80 or 160
// samples each. Up to 8 channels are allowed, 16, 20 or 24 bits/per sample.
// One frame may contain up to 128 subframes.
//
// To determine max frame size we may use max bitrate allowed = 18mbit
// (see http://en.wikipedia.org/wiki/Meridian_Lossless_Packing).
//
// 18 Mbit/s / 48000 samples/sec = 375bit/sample
// 375bit/sample * 40 samples/subframe * 128 subframes/frame = 240 KB/frame

class MlpBaseFrameParser : public BasicFrameParser
{
public:
  static const SyncTrie sync_trie;
  MlpBaseFrameParser() {}

  // Frame operations
  virtual bool      first_frame(const uint8_t *frame, size_t size);
  virtual bool      next_frame(const uint8_t *frame, size_t size);

protected:
  virtual SyncInfo build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const;
  bool check_frame_sequence(const uint8_t *frame, size_t size) const;
};

class MlpFrameParser : public MlpBaseFrameParser
{
public:
  static const SyncTrie sync_trie;
  MlpFrameParser() {}

  virtual bool      can_parse(int format) const { return format == FORMAT_MLP; }
  virtual SyncInfo  sync_info() const { return SyncInfo(sync_trie, 12, 240000); }

  // Frame header operations
  virtual size_t    header_size() const { return 12; }
  virtual bool      parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const;
  virtual bool      compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;
};

class TruehdFrameParser : public MlpBaseFrameParser
{
public:
  static const SyncTrie sync_trie;
  TruehdFrameParser() {}

  virtual bool      can_parse(int format) const { return format == FORMAT_TRUEHD; }
  virtual SyncInfo  sync_info() const { return SyncInfo(sync_trie, 12, 240000); }

  // Frame header operations
  virtual size_t    header_size() const { return 12; }
  virtual bool      parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const;
  virtual bool      compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;
};

#endif
