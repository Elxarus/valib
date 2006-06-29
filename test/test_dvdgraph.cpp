/*
  DVDGraph filter test
  * file transform test with format changes
  * dynamical user format change
  * dynamical graph rebuild
*/

#include "log.h"
#include "filter_tester.h"
#include "filters\dvd_graph.h"
#include <source\noise.h>
#include <source\raw_source.h>
#include <win32\cpu.h>
#include "common.h"

///////////////////////////////////////////////////////////////////////////////
// Test constants

static const vtime_t time_per_test = 1.0;    // 1 sec for each speed test
static const size_t min_data_size = 4096*12; // minimum data size to generate after state change:
                                             // size of 6-channel pcm16 data with more than maximum number of samples per frame

// noise speed test
static const int noise_size = 10000000;

class SinkAcceptSpdif : public NullSink
{
public:
  SinkAcceptSpdif() {};
};

class SinkRefuseSpdif : public NullSink
{
public:
  SinkRefuseSpdif() {};

  bool query_input(Speakers _spk) const
  { return _spk.format != FORMAT_SPDIF; }
};

///////////////////////////////////////////////////////////////////////////////
// Test class

class DVDGraph_test
{
protected:
  Filter *f;
  FilterTester t;
  DVDGraph dvd;
  Log *log;

public:
  DVDGraph_test(Log *_log)
  {
    log = _log;
    t.link(&dvd, log);
    f = &dvd; // do not use FilterTester
  }

  int test()
  {
    log->open_group("DVDGraph test");
    transform();
    user_format_change();
    spdif_rebuild();
    return log->close_group();
  }

  void transform()
  {
    /////////////////////////////////////////////////////////
    // Transform test
    log->open_group("Transform test");

    // PES to SPDIF transform with format changes
    dvd.set_sink(0);

    dvd.set_spdif(true, FORMAT_CLASS_SPDIFABLE, true);
    dvd.set_user(Speakers(FORMAT_PCM16_BE, 0, 0, 32768));

    compare_file(log, Speakers(FORMAT_PES, 0, 0), "a.mad.mix.pes",  f, "a.mad.mix.spdif");
    compare_file(log, Speakers(FORMAT_PES, 0, 0), "a.madp.mix.pes", f, "a.madp.mix.spdif");

    log->close_group();
  }

  void user_format_change()
  {
    log->open_group("Dynamical user format change");

    user_format_change("a.ac3.03f.ac3", Speakers(FORMAT_AC3, MODE_5_1, 48000));
    user_format_change("a.pcm.005.lpcm", Speakers(FORMAT_PCM16_BE, MODE_STEREO, 48000));

    log->close_group();
  }

  int user_format_change(const char *file_name, Speakers spk)
  {
    Speakers formats[] = {
      Speakers(FORMAT_PCM16, 0, 0),            // pcm16 as-is
      Speakers(FORMAT_PCM16, MODE_STEREO, 0),  // pcm16 stereo (possible downmix)
      Speakers(FORMAT_PCM16, MODE_5_1, 0),     // pcm16 5.1 (possible upmix)
      Speakers(FORMAT_PCM32, 0, 0),            // pcm32 as-is
      Speakers(FORMAT_PCM32, MODE_STEREO, 0),  // pcm32 stereo (possible downmix)
      Speakers(FORMAT_PCM32, MODE_5_1, 0),     // pcm32 5.1 (possible upmix)
      // spdif pt
      Speakers(FORMAT_PCM16, 0, 0),            // pcm16 as-is
      Speakers(FORMAT_PCM16, MODE_STEREO, 0),  // pcm16 stereo (possible downmix)
      Speakers(FORMAT_PCM16, MODE_5_1, 0),     // pcm16 5.1 (possible upmix)
      Speakers(FORMAT_PCM32, 0, 0),            // pcm32 as-is
      Speakers(FORMAT_PCM32, MODE_STEREO, 0),  // pcm32 stereo (possible downmix)
      Speakers(FORMAT_PCM32, MODE_5_1, 0),     // pcm32 5.1 (possible upmix)
    };
    bool spdif_stereo_pt[] = {
      false, false, false, false, false, false, 
      true,  true,  true,  true,  true,  true,
    };

    log->open_group("Testing %s (%s %s %i)", file_name, 
      spk.format_text(), spk.mode_text(), spk.sample_rate);

    Chunk chunk;
    RAWSource src(spk, file_name, 2048);
    if (!src.is_open())
      return log->err_close("Cannot open file %s", file_name);

    for (int i = 0; i < array_size(formats); i++)
    {
      dvd.set_user(formats[i]);
      dvd.set_spdif(spdif_stereo_pt[i], 0, spdif_stereo_pt[i]);

      Speakers test_spk = spk;
      if (formats[i].format)  test_spk.format = formats[i].format;
      if (formats[i].mask)    test_spk.mask   = formats[i].mask;

      if (spdif_stereo_pt[i] && test_spk.mask != MODE_STEREO)
        test_spk.format = FORMAT_SPDIF;

      while (!src.is_empty() && f->get_output() != test_spk)
        if (f->is_empty())
        {
          if (!src.get_chunk(&chunk))
            return log->err_close("src->get_chunk() failed");

          if (!f->process(&chunk))
            return log->err_close("dvd.process() failed");
        }
        else
        {
          if (!f->get_chunk(&chunk))
            return log->err_close("dvd.get_chunk() failed");
        }

      if (f->get_output() != test_spk)
        return log->err_close("cannot change format: %s%s %s -> %s %s",
          spdif_stereo_pt[i]? "(SPDIF/stereo passthrough) ": "",
          formats[i].format? formats[i].format_text(): "as-is",
          formats[i].mask? formats[i].mode_text(): "as-is",
          test_spk.format_text(), test_spk.mode_text());
      else
        log->msg("successful format change: %s%s %s -> %s %s",
          spdif_stereo_pt[i]? "(SPDIF/stereo passthrough) ": "",
          formats[i].format? formats[i].format_text(): "as-is",
          formats[i].mask? formats[i].mode_text(): "as-is",
          test_spk.format_text(), test_spk.mode_text());
    }

    return log->close_group();
  }

  void spdif_rebuild()
  {
    /////////////////////////////////////////////////////////
    // Dynamical graph rebuild
    log->open_group("Dynamical graph rebuild");

    // Raw streams
    spdif_rebuild("a.ac3.03f.ac3", Speakers(FORMAT_AC3, 0, 0), true, true);
    spdif_rebuild("a.dts.03f.dts", Speakers(FORMAT_DTS, 0, 0), true, true);
    spdif_rebuild("a.mp2.005.mp2", Speakers(FORMAT_MPA, 0, 0), true, true);

    // PES streams
    spdif_rebuild("a.ac3.03f.pes", Speakers(FORMAT_PES, 0, 0), true, true);
    spdif_rebuild("a.dts.03f.pes", Speakers(FORMAT_PES, 0, 0), true, true);
    spdif_rebuild("a.mp2.005.pes", Speakers(FORMAT_PES, 0, 0), true, true);

    // PCM streams
    spdif_rebuild("a.pcm.005.lpcm", Speakers(FORMAT_PCM16_BE, MODE_STEREO, 48000), false, true);
    spdif_rebuild("a.pcm.005.lpcm", Speakers(FORMAT_PCM16_BE, MODE_STEREO, 45000), false, false);
    spdif_rebuild("a.pcm.005.pes",  Speakers(FORMAT_PES, 0, 0), false, true);

    log->close_group();
  }

  int spdif_rebuild(const char *file_name, Speakers spk, bool is_spdifable, bool can_encode)
  {
    log->open_group("Testing %s (%s)%s%s", 
      file_name, spk.format_text(), 
      is_spdifable? "": " (not spdifable)",
      can_encode? "": " (cannot encode)");

    RAWSource src(spk, file_name, 2048);
    if (!src.is_open())
      return log->err_close("Cannot open file %s", file_name);

    if (!f->set_input(spk))
      return log->err_close("dvd.set_input(%s %s %iHz) failed", 
        spk.format_text(), spk.mode_text(), spk.sample_rate);

    // Check all sink modes
    // (no sink, spdif allowed, spdif refused)

    SinkAcceptSpdif sink_accept_spdif;
    SinkRefuseSpdif sink_refuse_spdif;
    const Sink *sink[] = { 0, &sink_accept_spdif, &sink_refuse_spdif };
    const bool spdif_allowed[] = { true, true, false };
    const char *sink_name[] = { "no sink", "sink that accepts spdif", "sink that refuses spdif" };

    for (int isink = 0; isink < array_size(sink); isink++)
    {
      log->msg("Test with %s", sink_name[isink]);
      dvd.set_sink(sink[isink]);

      // Check all possible transition between spdif modes
      // (decode, passthrough, encode, stereo passthrough)

      if (is_spdifable)
      {
        test_decode(&src);
        test_passthrough(&src, spdif_allowed[isink]);
        test_encode(&src, spdif_allowed[isink], can_encode);
        test_stereo_passthrough(&src);

        test_decode(&src);
        test_encode(&src, spdif_allowed[isink], can_encode);
        test_decode(&src);

        test_stereo_passthrough(&src);
        test_passthrough(&src, spdif_allowed[isink]);
        test_stereo_passthrough(&src);

        test_encode(&src, spdif_allowed[isink], can_encode);
        test_passthrough(&src, spdif_allowed[isink]);
        test_decode(&src);
      }
      else
      {
        test_decode(&src);
        test_encode(&src, spdif_allowed[isink], can_encode);
        test_stereo_passthrough(&src);
        test_decode(&src);
        test_stereo_passthrough(&src);
        test_encode(&src, spdif_allowed[isink], can_encode);
        test_decode(&src);
      }
    }

    Chunk chunk;
    while (!src.is_empty() || !f->is_empty())
    {
      if (f->is_empty())
      {
        if (!src.get_chunk(&chunk))
          return log->err_close("src.get_chunk() failed");
        if (!f->process(&chunk))
          return log->err_close("dvd.process() failed");
      }
      else
      {
        if (!f->get_chunk(&chunk))
          return log->err_close("dvd.get_chunk() failed");
      }
    }

    // todo: check number of output streams
    return log->close_group();
  }

  int test_decode(Source *src)
  {
    dvd.set_user(Speakers(FORMAT_PCM16, 0, 0, 32768));
    dvd.set_spdif(false, 0, false);
    return test_cycle("test_decode()", src, SPDIF_DISABLED, "decode", FORMAT_PCM16);
  }

  int test_passthrough(Source *src, bool spdif_allowed)
  {
    dvd.set_user(Speakers(FORMAT_PCM16, 0, 0, 32768));
    dvd.set_spdif(true, FORMAT_CLASS_SPDIFABLE, false);
    if (spdif_allowed)
      return test_cycle("test_passthrough()", src, SPDIF_PASSTHROUGH, "spdif passthrough", FORMAT_SPDIF);
    else
      return test_cycle("test_passthrough()", src, SPDIF_DISABLED, "sink refused", FORMAT_PCM16);
  }

  int test_encode(Source *src, bool spdif_allowed, bool can_encode)
  {
    dvd.set_user(Speakers(FORMAT_PCM16, 0, 0, 32768));
    dvd.set_spdif(true, 0, false);

    if (!can_encode)
      return test_cycle("test_encode()", src, SPDIF_DISABLED, "cannot encode", FORMAT_PCM16);

    if (spdif_allowed)
      return test_cycle("test_encode()", src, SPDIF_ENCODE, "ac3 encode", FORMAT_SPDIF);
    else
      return test_cycle("test_encode()", src, SPDIF_DISABLED, "sink refused", FORMAT_PCM16);
  }

  int test_stereo_passthrough(Source *src)
  {
    dvd.set_user(Speakers(FORMAT_PCM16, MODE_STEREO, 0, 32768));
    dvd.set_spdif(true, 0, true);
    return test_cycle("test_stereo_passthrough()", src, SPDIF_DISABLED, "stereo pcm passthrough", FORMAT_PCM16);
  }

  int test_cycle(const char *caller, Source *src, int status, const char *status_text, int out_format)
  {
    Chunk chunk;

    // process until status change
    // (filter may be either full or empty)

    while (!src->is_empty() && dvd.get_spdif_status() != status)
      if (f->is_empty())
      {
        if (!src->get_chunk(&chunk))
          return log->err("%s: src->get_chunk() failed before status change", caller);

        if (!f->process(&chunk))
          return log->err("%s: dvd.process() failed before status change", caller);
      }
      else
      {
        if (!f->get_chunk(&chunk))
          return log->err("%s: dvd.get_chunk() failed before status change", caller);
      }

    if (dvd.get_spdif_status() != status)
      return log->err("%s: cannot switch to %s state", caller, status_text);

    // ensure correct data generation in new state
    // (filter may be either full or empty)

    size_t data_size = 0;
    while (!src->is_empty() && data_size < min_data_size)
      if (f->is_empty())
      {
        if (!src->get_chunk(&chunk))
          return log->err("%s: src->get_chunk() failed after status change", caller);

        if (!f->process(&chunk))
          return log->err("%s: dvd.process() failed after status change", caller);
      }
      else
      {
        if (!f->get_chunk(&chunk))
          return log->err("%s: dvd.get_chunk() failed after status change", caller);

        if (!chunk.is_dummy())
          if (chunk.spk.format != out_format)
            return log->err("%s: incorrect output format", caller);
        data_size += chunk.size;
      }

    // well done...
    // (filter may be either full or empty)
    return 0;
  }
};

///////////////////////////////////////////////////////////////////////////////
// Test function

int test_dvdgraph(Log *log)
{
  DVDGraph_test test(log);
  return test.test();
}
