/*
  SPDIFWrapper test
*/

#include <boost/test/unit_test.hpp>
#include "filters/filter_graph.h"
#include "parsers/dts/dts_header.h"
#include "parsers/spdif/spdif_header.h"
#include "parsers/spdif/spdif_parser.h"
#include "parsers/spdif/spdif_wrapper.h"
#include "parsers/spdif/spdifable_header.h"
#include "source/file_parser.h"
#include "../../../suite.h"
#include "dts_frame_resize.h"

static const char *dts_mode_text(int i)
{
  switch (i)
  {
  case DTS_MODE_AUTO: return "Auto";
  case DTS_MODE_WRAPPED: return "Wrapped";
  case DTS_MODE_PADDED: return "Padded";
  default: return "?";
  }
}

static const char *dts_conv_text(int i)
{
  switch (i)
  {
  case DTS_CONV_NONE: return "None";
  case DTS_CONV_14BIT: return "14bit";
  case DTS_CONV_16BIT: return "16bit";
  default: return "?";
  }
}

static void compare_file(const char *raw_file, const char *spdif_file)
{
  BOOST_MESSAGE("Transform " << spdif_file << " -> " << raw_file);

  FileParser f_raw;
  SpdifableFrameParser frame_parser_raw;
  f_raw.open_probe(raw_file, &frame_parser_raw);
  BOOST_REQUIRE(f_raw.is_open());

  FileParser f_spdif;
  SPDIFFrameParser frame_parser_spdif;
  f_spdif.open_probe(spdif_file, &frame_parser_spdif);
  BOOST_REQUIRE(f_spdif.is_open());

  SPDIFWrapper spdifer;
  compare(&f_raw, &spdifer, &f_spdif, 0);
}

static void test_streams_frames(const char *filename, int streams, int frames)
{
  BOOST_MESSAGE("Count frames at " << filename);

  FileParser f;
  SpdifableFrameParser frame_parser;
  f.open_probe(filename, &frame_parser);
  BOOST_REQUIRE(f.is_open());

  SPDIFWrapper parser;
  parser.open(f.get_output());
  BOOST_CHECK(parser.is_open());

  check_streams_chunks(&f, &parser, streams, frames);
}

BOOST_AUTO_TEST_SUITE(spdif_wrapper)

BOOST_AUTO_TEST_CASE(constructor)
{
  SPDIFWrapper spdifer;
  BOOST_CHECK_EQUAL(spdifer.dts_mode, DTS_MODE_AUTO);
  BOOST_CHECK_EQUAL(spdifer.dts_conv, DTS_CONV_NONE);
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  SPDIFWrapper spdifer(DTS_MODE_WRAPPED, DTS_CONV_14BIT);
  BOOST_CHECK_EQUAL(spdifer.dts_mode, DTS_MODE_WRAPPED);
  BOOST_CHECK_EQUAL(spdifer.dts_conv, DTS_CONV_14BIT);
}

BOOST_AUTO_TEST_CASE(can_open)
{
  SPDIFWrapper spdifer;
  BOOST_CHECK(spdifer.can_open(Speakers(FORMAT_AC3, 0, 0)));
  BOOST_CHECK(spdifer.can_open(Speakers(FORMAT_DTS, 0, 0)));
  BOOST_CHECK(spdifer.can_open(Speakers(FORMAT_MPA, 0, 0)));
}

BOOST_AUTO_TEST_CASE(parse)
{
  compare_file("a.mp2.005.mp2", "a.mp2.005.spdif");
  compare_file("a.mp2.002.mp2", "a.mp2.002.spdif");
  compare_file("a.ac3.03f.ac3", "a.ac3.03f.spdif");
  compare_file("a.ac3.005.ac3", "a.ac3.005.spdif");
  compare_file("a.dts.03f.dts", "a.dts.03f.spdif");
  compare_file("a.mad.mix.mad", "a.mad.mix.spdif");
}

BOOST_AUTO_TEST_CASE(streams_frames)
{
  test_streams_frames("a.mp2.005.mp2", 1, 500);
  test_streams_frames("a.mp2.002.mp2", 1, 500);
  test_streams_frames("a.ac3.03f.ac3", 1, 375);
  test_streams_frames("a.ac3.005.ac3", 1, 375);
  test_streams_frames("a.dts.03f.dts", 1, 1125);
  test_streams_frames("a.mad.mix.mad", 7, 4375);
}

BOOST_AUTO_TEST_CASE(dts_options)
{
  enum mode_t { mode_wrap, mode_pad, mode_pass };
  struct {
    int dts_mode;
    int dts_conv;
    const char *filename;
    const char *ref_filename;
    size_t frame_size;
    size_t ref_frame_size;
    mode_t mode;
  } tests[] = {
    { DTS_MODE_AUTO, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", 2032, 2032, mode_wrap },
    { DTS_MODE_AUTO, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", 2033, 2048, mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", 2047, 2048, mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", 2049, 2049, mode_pass },

    { DTS_MODE_AUTO, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", 2032, 2032, mode_wrap },
    { DTS_MODE_AUTO, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", 2033, 2048, mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", 2047, 2048, mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", 2049, 2049, mode_pass },

    { DTS_MODE_AUTO, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", 2032, 2032, mode_wrap },
    { DTS_MODE_AUTO, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", 2033, 2048, mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", 2047, 2048, mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", 2049, 2049, mode_pass },

    { DTS_MODE_AUTO, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts",   2320, 2030, mode_wrap },
    { DTS_MODE_AUTO, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts",   2328, 2048, mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts",   2336, 2048, mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", 2344, 2344, mode_pass },

    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts14", 1778, 2032, mode_wrap },
    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts14", 1779, 2048, mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts14", 1792, 2048, mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts",   1793, 1793, mode_wrap },
    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts",   2032, 2032, mode_wrap },
    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts",   2033, 2048, mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts",   2047, 2048, mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts",   2049, 2049, mode_pass },

    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", 2032, 2032, mode_wrap },
    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", 2033, 2048, mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", 2047, 2048, mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", 2049, 2049, mode_pass },

    { DTS_MODE_WRAPPED, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", 2032, 2032, mode_wrap },
    { DTS_MODE_WRAPPED, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", 2047, 2047, mode_pass },

    { DTS_MODE_WRAPPED, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", 2032, 2032, mode_wrap },
    { DTS_MODE_WRAPPED, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", 2047, 2047, mode_pass },

    { DTS_MODE_WRAPPED, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", 2032, 2032, mode_wrap },
    { DTS_MODE_WRAPPED, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", 2047, 2047, mode_pass },

    { DTS_MODE_WRAPPED, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts",   2320, 2030, mode_wrap },
    { DTS_MODE_WRAPPED, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", 2336, 2336, mode_pass },

    { DTS_MODE_WRAPPED, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts14", 1778, 2032, mode_wrap },
    { DTS_MODE_WRAPPED, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts",   1779, 1779, mode_wrap },
    { DTS_MODE_WRAPPED, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts",   2032, 2032, mode_wrap },
    { DTS_MODE_WRAPPED, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts",   2033, 2033, mode_pass },

    { DTS_MODE_WRAPPED, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", 2032, 2032, mode_wrap },
    { DTS_MODE_WRAPPED, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", 2040, 2040, mode_pass },

    { DTS_MODE_PADDED, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", 2047, 2048, mode_pad  },
    { DTS_MODE_PADDED, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", 2049, 2049, mode_pass },

    { DTS_MODE_PADDED, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", 2047, 2048, mode_pad  },
    { DTS_MODE_PADDED, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", 2049, 2049, mode_pass },

    { DTS_MODE_PADDED, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", 2047, 2048, mode_pad  },
    { DTS_MODE_PADDED, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", 2049, 2049, mode_pass },

    { DTS_MODE_PADDED, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts",   2336, 2048, mode_pad  },
    { DTS_MODE_PADDED, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", 2344, 2344, mode_pass },

    { DTS_MODE_PADDED, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts14", 1792, 2048, mode_pad  },
    { DTS_MODE_PADDED, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts",   1793, 2048, mode_pad  },
    { DTS_MODE_PADDED, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts",   2047, 2048, mode_pad  },
    { DTS_MODE_PADDED, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts",   2049, 2049, mode_pass },

    { DTS_MODE_PADDED, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", 2040, 2048, mode_pad  },
    { DTS_MODE_PADDED, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", 2056, 2056, mode_pass },
  };

  for (int i = 0; i < array_size(tests); i++)
  {
    // Test chain: FrameParser->DTSFrameResizer->Spdifer->[Despdifer]
    // Reference chain: FrameParser->DTSFrameResizer

    FileParser f_test;
    DTSFrameParser frame_parser_test;
    f_test.open_probe(tests[i].filename, &frame_parser_test);
    BOOST_REQUIRE(f_test.is_open());

    FileParser f_ref;
    DTSFrameParser frame_parser_ref;
    f_ref.open_probe(tests[i].ref_filename, &frame_parser_ref);
    BOOST_REQUIRE(f_ref.is_open());

    DTSFrameResize resize(tests[i].frame_size);
    SPDIFWrapper spdifer(tests[i].dts_mode, tests[i].dts_conv);
    SPDIFParser despdifer;

    FilterChain test(&resize, &spdifer);
    if (tests[i].mode != mode_pass)
      test.add_back(&despdifer);

    DTSFrameResize ref_resize(tests[i].ref_frame_size);

    BOOST_MESSAGE("test: " << i << " dts_mode: " << dts_mode_text(tests[i].dts_mode) << " dts_conv: " << dts_conv_text(tests[i].dts_conv));
    compare(&f_test, &test, &f_ref, &ref_resize);
  }
}

BOOST_AUTO_TEST_SUITE_END()
