#ifndef PES_HEADER_H
#define PES_HEADER_H

#include "../../spk.h"

struct PESHeader
{
  size_t packet_size;

  int stream;
  int substream;
  size_t substream_header_pos;

  Speakers spk;
  size_t payload_pos;
  size_t payload_size;

  bool parse(const uint8_t *header);
  bool operator == (const PESHeader &other) const;
  bool operator != (const PESHeader &other) const;

  static bool is_audio_stream(int stream);
};

#endif
