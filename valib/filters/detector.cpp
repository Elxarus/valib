#include "detector.h"
#include "../parsers/uni/uni_frame_parser.h"


Detector::Detector()
{
  out_spk = spk_unknown;
  state = state_load;
  do_flush = false;
}

FrameParser *
Detector::find_parser(Speakers spk)
{
  int format = spk.format;
  if (spk.format == FORMAT_PCM16)
    format = FORMAT_SPDIF;

  return uni_parser.find_parser(format);
}

bool
Detector::can_open(Speakers new_spk) const
{
  // Only stereo PCM16 is allowed for SPDIF detection
  if (new_spk.format == FORMAT_PCM16)
    return new_spk.mask == MODE_STEREO && new_spk.sample_rate > 0;

  return uni_parser.can_parse(new_spk.format);
}

bool
Detector::init()
{
  FrameParser *parser = find_parser(spk);
  if (!parser)
    return false;

  stream.set_parser(parser);
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
Detector::process(Chunk &in, Chunk &out)
{
  sync.receive_sync(in);

  do_flush = true;
  is_new_stream = false;
  while (1) switch (state)
  {
    case state_load:
      load(in);

      if (stream.is_in_sync())
      {
        if (out_spk.is_unknown() || stream.is_new_stream())
          is_new_stream = true;
        out_spk = stream.get_spk();

        if (stream.has_debris())
        {
          out.set_rawdata(stream.get_debris(), stream.get_debris_size());
          state = state_frame;
          return true;
        }
        else if (stream.has_frame())
        {
          out.set_rawdata(stream.get_frame(), stream.get_frame_size());
          sync.send_frame_sync(out);
          return true;
        }
        else
          return false;
      }
      else if (stream.has_debris())
      {
        if (out_spk.is_unknown() || out_spk != spk)
          is_new_stream = true;
        out_spk = spk;

        out.set_rawdata(stream.get_debris(), stream.get_debris_size());
        if (spk.format == FORMAT_PCM16)
          sync.send_sync(out, 1.0/(4 * spk.sample_rate));
        else
          sync.send_frame_sync(out);
        return true;
      }
      else
        return false;

    case state_frame:
      assert(stream.has_frame());
      out.set_rawdata(stream.get_frame(), stream.get_frame_size());
      sync.send_frame_sync(out);
      state = state_load;
      return true;
  }

  assert(false);
  return false;
}

bool
Detector::flush(Chunk &out)
{
  if (!do_flush)
    return false;

  do_flush = true;
  is_new_stream = false;
  while (1) switch (state)
  {
    case state_load:
      stream.flush();

      if (stream.is_in_sync())
      {
        if (out_spk.is_unknown() || stream.is_new_stream())
          is_new_stream = true;
        out_spk = stream.get_spk();

        if (stream.has_debris())
        {
          out.set_rawdata(stream.get_debris(), stream.get_debris_size());
          state = state_frame;
          return true;
        }
        else if (stream.has_frame())
        {
          out.set_rawdata(stream.get_frame(), stream.get_frame_size());
          sync.send_frame_sync(out);
          return true;
        }
      }
      else if (stream.has_debris())
      {
        if (out_spk.is_unknown() || out_spk != spk)
          is_new_stream = true;
        out_spk = spk;

        out.set_rawdata(stream.get_debris(), stream.get_debris_size());
        if (spk.format == FORMAT_PCM16)
          sync.send_sync(out, 1.0/(4 * spk.sample_rate));
        else
          sync.send_frame_sync(out);
        return true;
      }

      do_flush = false;
      is_new_stream = false;
      return false;

    case state_frame:
      assert(stream.has_frame());
      out.set_rawdata(stream.get_frame(), stream.get_frame_size());
      sync.send_frame_sync(out);
      state = state_load;
      return true;
  }

  switch (state)
  {
    case state_load:
      do_flush = false;
      is_new_stream = false;

      stream.flush();
      if (!stream.has_debris())
        return false;

      out.set_rawdata(stream.get_debris(), stream.get_debris_size());
      if (out_spk.format == FORMAT_PCM16)
        sync.send_sync(out, 1.0/(4 * spk.sample_rate));
      else
        sync.send_frame_sync(out);
      sync.reset();
      return true;

    case state_frame:
      // We have data in the buffer
      // Do dummy processing
      Chunk dummy;
      return process(dummy, out);
  }
  assert(false);
  return false;
}

void
Detector::load(Chunk &in)
{
  uint8_t *buf = in.rawdata;
  uint8_t *end = buf + in.size;
  size_t old_data_size = stream.get_buffer_size();

  stream.load(&buf, end);
  size_t gone = buf - in.rawdata;
  in.drop_rawdata(gone);

  sync.put(gone);
  sync.drop(old_data_size + gone - stream.get_buffer_size());
}
