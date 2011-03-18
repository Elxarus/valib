#include "detector.h"
#include "../parsers/spdif/spdif_header.h"
#include "../parsers/aac/aac_adts_header.h"
#include "../parsers/ac3/ac3_header.h"
#include "../parsers/ac3_eac3/ac3_eac3_header.h"
#include "../parsers/dts/dts_header.h"
#include "../parsers/eac3/eac3_header.h"
#include "../parsers/mpa/mpa_header.h"


Detector::Detector()
{
  static const HeaderParser *uni_parsers[] =
  {
    &spdif_header,
    &adts_header,
    &ac3_header,
    ac3_eac3_header(),
    &dts_header,
    &eac3_header,
    &mpa_header
  };

  out_spk = spk_unknown;
  state = state_load;
  do_flush = false;

  uni_header.set_parsers(uni_parsers, array_size(uni_parsers));
}

const HeaderParser *
Detector::find_parser(Speakers spk) const
{
  switch (spk.format)
  {
    case FORMAT_RAWDATA: return &uni_header;
    case FORMAT_PCM16:   return &spdif_header;
    case FORMAT_SPDIF:   return &spdif_header;

    case FORMAT_AAC_ADTS:return &adts_header;
    case FORMAT_AC3:     return &ac3_header;
    case FORMAT_AC3_EAC3:return ac3_eac3_header();
    case FORMAT_DTS:     return &dts_header;
    case FORMAT_EAC3:    return &eac3_header;
    case FORMAT_MPA:     return &mpa_header;
  };

  return 0;
}

bool
Detector::can_open(Speakers new_spk) const
{
  // Only stereo PCM16 is allowed for SPDIF detection
  if (new_spk.format == FORMAT_PCM16)
    return new_spk.mask == MODE_STEREO && new_spk.sample_rate > 0;

  return find_parser(new_spk) != 0;
}

bool
Detector::init()
{
  const HeaderParser *hparser = find_parser(spk);
  if (!hparser)
    return false;

  stream.set_parser(hparser);
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
