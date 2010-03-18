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
  state = state_trans;
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
Detector::init(Speakers new_spk)
{
  const HeaderParser *hparser = find_parser(new_spk);
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
  state = state_trans;
  do_flush = false;

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

  if (!in.size)
    return true;

  do_flush = true;
  switch (state)
  {
    ///////////////////////////////////////////////////////
    // Transition state
    // Initial state of the detector. No data was loaded
    // and output format is not known (out_spk = spk_unknown).
    // Load the stream buffer and switch to either:
    // * sync mode on successful synchronization
    // * passthrough mode if we cannot sync
   
    case state_trans:
      load(in);

      if (stream.is_new_stream())
      {
        out_spk = stream.get_spk();
        if (stream.is_debris_exists())
        {
          out.set_rawdata(stream.get_debris(), stream.get_debris_size());
          state = state_frame;
        }
        else
        {
          out.set_rawdata(stream.get_frame(), stream.get_frame_size());
          sync.send_frame_sync(out);
          state = state_next_frame;
        }
      }
      else if (stream.is_debris_exists())
      {
        out_spk = spk;
        out.set_rawdata(stream.get_debris(), stream.get_debris_size());
        sync.send_frame_sync(out);
        state = state_passthrough;
      }
      else
        out.set_empty();

      return true;

    ///////////////////////////////////////////////////////
    // Passthrough state
    // No synchronization found. Output format equals to
    // input format. Load data into the stream buffer and:
    // * passthrough if it is no sync
    // * format change mode on successful synchronization

    case state_passthrough:
      assert(!stream.is_in_sync());
      load(in);

      if (stream.is_new_stream())
      {
        out.set_empty();
        state = state_format_change;
      }
      else
      {
        out.set_rawdata(stream.get_debris(), stream.get_debris_size());
        sync.send_frame_sync(out);
      }
      return true;

    ///////////////////////////////////////////////////////
    // Format change

    case state_format_change:
      assert(stream.is_new_stream());

      out_spk = stream.get_spk();
      if (stream.is_debris_exists())
      {
        out.set_rawdata(stream.get_debris(), stream.get_debris_size());
        state = state_frame;
      }
      else
      {
        out.set_rawdata(stream.get_frame(), stream.get_frame_size());
        sync.send_frame_sync(out);
        state = state_next_frame;
      }
      return true;

    ///////////////////////////////////////////////////////
    // Sync lost

    case state_sync_lost:
      assert(!stream.is_in_sync());

      if (stream.is_debris_exists())
      {
        out_spk = spk;
        out.set_rawdata(stream.get_debris(), stream.get_debris_size());
        sync.send_frame_sync(out);
        state = state_passthrough;
      }
      else
      {
        out_spk = spk_unknown;
        out.set_empty();
        state = state_trans;
      }
      return true;

    ///////////////////////////////////////////////////////
    // Next frame
    // We're in sync. Load a next frame and send either
    // debris or frame. Handle sync lost and new stream
    // conditions.

    case state_next_frame:
      assert(stream.is_in_sync());
      load(in);

      if (!stream.is_in_sync())
      {
        out.set_empty();
        state = state_sync_lost;
      }
      else if (stream.is_new_stream())
      {
        out.set_empty();
        state = state_format_change;
      }
      else if (stream.is_debris_exists())
      {
        out.set_rawdata(stream.get_debris(), stream.get_debris_size());
        if (stream.is_frame_loaded())
          state = state_frame;
      }
      else if (stream.is_frame_loaded())
      {
        out.set_rawdata(stream.get_frame(), stream.get_frame_size());
        sync.send_frame_sync(out);
      }
      else
        out.set_empty();

      return true;

    ///////////////////////////////////////////////////////
    // Frame output
    // Frame was loaded but we didn't send it. Send it and
    // switch to next frmae state.

    case state_frame:
      assert(stream.is_frame_loaded());
      out.set_rawdata(stream.get_frame(), stream.get_frame_size());
      state = state_next_frame;
      return true;
  }

  assert(false);
  return false;
}

bool
Detector::flush(Chunk2 &out)
{
  switch (state)
  {
    case state_trans:
    case state_passthrough:
    case state_next_frame:
      stream.flush();
      if (stream.is_debris_exists())
        out.set_rawdata(stream.get_debris(), stream.get_debris_size());
      else
        out.set_empty();

      state = state_sync_lost;
      do_flush = false;
      return true;

    case state_format_change:
    case state_sync_lost:
    case state_frame:
      // We have data in the stream buffer
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
  uint8_t *end = in.rawdata + in.size;
  stream.load(&in.rawdata, end);
  in.size = end - in.rawdata;

  if (stream.is_debris_exists())
    sync.drop(stream.get_debris_size());

  if (stream.is_frame_loaded())
    sync.drop(stream.get_frame_size());
}
