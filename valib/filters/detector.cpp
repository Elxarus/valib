#include <stdio.h>
#include "detector.h"
#include "../parsers/spdif/spdif_header.h"
#include "../parsers/mpa/mpa_header.h"
#include "../parsers/ac3/ac3_header.h"
#include "../parsers/dts/dts_header.h"


static const HeaderParser *spdif_dts_parsers[] =
{
  &spdif_header,
  &dts_header
};

static const HeaderParser *uni_parsers[] =
{
  &spdif_header,
  &ac3_header,
  &dts_header,
  &mpa_header
};


Detector::Detector()
{
  out_spk = spk_unknown;
  state = state_load;
  do_flush = false;

  spdif_dts_header.set_parsers(spdif_dts_parsers, array_size(spdif_dts_parsers));
  uni_header.set_parsers(uni_parsers, array_size(uni_parsers));
}

Detector::~Detector()
{
}

const HeaderParser *
Detector::find_parser(Speakers spk) const
{
  switch (spk.format)
  {
    case FORMAT_RAWDATA: return &uni_header;
    case FORMAT_PCM16:   return &spdif_dts_header;
    case FORMAT_SPDIF:   return &spdif_dts_header;

    case FORMAT_AC3:     return &ac3_header;
    case FORMAT_DTS:     return &dts_header;
    case FORMAT_MPA:     return &mpa_header;
  };

  return 0;
}

bool
Detector::can_open(Speakers new_spk) const
{
  return find_parser(new_spk) != 0;
}

bool
Detector::init()
{
  const HeaderParser *hparser = find_parser(spk);
  if (!hparser)
    return false;

  if (!stream.set_parser(hparser))
    return false;

  reset();
  return true;
}

void
Detector::reset()
{
  out_spk = spk_unknown;
  state = state_load;
  do_flush = false;
  is_new_stream = false;

  stream.reset();
  sync.reset();
}

bool
Detector::process(Chunk2 &in, Chunk2 &out)
{
  if (in.sync)
  {
    sync.receive_sync(in, stream.get_buffer_size());
    in.sync = false;
    in.time = 0;
  }

  do_flush = true;
  is_new_stream = false;
  while (1) switch (state)
  {
    case state_load:
      load(in);

      if (stream.is_in_sync())
      {
        if (!out_spk.is_unknown() && stream.is_new_stream())
          is_new_stream = true;
        out_spk = stream.get_spk();

        if (stream.is_debris_exists())
        {
          out.set_rawdata(stream.get_debris(), stream.get_debris_size());
          state = state_frame;
          return true;
        }
        else if (stream.is_frame_loaded())
        {
          out.set_rawdata(stream.get_frame(), stream.get_frame_size());
          sync.send_frame_sync(out);
          return true;
        }
        else
          return false;
      }
      else if (stream.is_debris_exists())
      {
        if (!out_spk.is_unknown() && out_spk != spk)
          is_new_stream = true;
        out_spk = spk;

        out.set_rawdata(stream.get_debris(), stream.get_debris_size());
        sync.send_frame_sync(out);
        return true;
      }
      else
        return false;

    case state_frame:
      assert(stream.is_frame_loaded());
      out.set_rawdata(stream.get_frame(), stream.get_frame_size());
      sync.send_frame_sync(out);
      state = state_load;
      return true;
  }

  assert(false);
  return false;
}

bool
Detector::flush(Chunk2 &out)
{
  if (!do_flush)
    return false;

  switch (state)
  {
    case state_load:
      stream.flush();
      if (stream.is_debris_exists())
        out.set_rawdata(stream.get_debris(), stream.get_debris_size());
      else
        out.set_empty();

      do_flush = false;
      is_new_stream = false;
      return true;

    case state_frame:
      // We have data in the buffer
      // Do dummy processing
      Chunk2 dummy;
      return process(dummy, out);
  }
  assert(false);
  return false;
}

void
Detector::load(Chunk2 &in)
{
  size_t old_data_size = stream.get_buffer_size() + in.size;

  uint8_t *end = in.rawdata + in.size;
  stream.load(&in.rawdata, end);
  in.size = end - in.rawdata;

  size_t new_data_size = stream.get_buffer_size() + in.size;
  sync.drop(old_data_size - new_data_size);
}
