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

// Fitler that pads DTS frames with zeros to match certain size
class DTSFrameResize : public SimpleFilter
{
public:
  DTSFrameResize()
  {}

  DTSFrameResize(size_t frame_size): frame(frame_size)
  {}

  void set(size_t frame_size)
  { frame.allocate(frame_size); }

  bool can_open(Speakers spk) const
  { return spk.format == FORMAT_DTS; }

  bool process(Chunk &in, Chunk &out)
  {
    HeaderInfo hinfo;
    Chunk temp = in;
    in.clear();
    if (!frame.is_allocated())
    {
      // passthrough
      out = temp;
      return true;
    }

    if (temp.size < dts_header.header_size())
      return false;
    if (!dts_header.parse_header(temp.rawdata, &hinfo))
      return false;
    if (temp.size < hinfo.frame_size || in.size > frame.size())
      return false;

    memcpy(frame.begin(), temp.rawdata, temp.size);
    memset(frame.begin() + temp.size, 0, frame.size() - temp.size);
    out.set_rawdata(frame.begin(), frame.size(), temp.sync, temp.time);
    return true;
  }

protected:
  Rawdata frame;
};

BOOST_AUTO_TEST_SUITE(spdif_wrapper)

BOOST_AUTO_TEST_CASE(constructor)
{
  SPDIFWrapper spdifer;
}

BOOST_AUTO_TEST_CASE(parse)
{
  FileParser f_raw;
  f_raw.open_probe("a.mad.mix.mad", &spdifable_header);
  BOOST_REQUIRE(f_raw.is_open());

  FileParser f_spdif;
  f_spdif.open_probe("a.mad.mix.spdif", &spdif_header);
  BOOST_REQUIRE(f_spdif.is_open());

  SPDIFWrapper spdifer;
  compare(&f_raw, &spdifer, &f_spdif, 0);
}

BOOST_AUTO_TEST_CASE(streams_frames)
{
  FileParser f;
  f.open_probe("a.mad.mix.mad", &spdifable_header);
  BOOST_REQUIRE(f.is_open());

  SPDIFWrapper parser;
  parser.open(f.get_output());
  BOOST_CHECK(parser.is_open());

  check_streams_chunks(&f, &parser, 7, 4375);
}

BOOST_AUTO_TEST_CASE(dts_options)
{
  static const size_t fs = 2048; // SPDIF frame size
  static const size_t hs = 16; // SPDIF header size
  enum mode_t { mode_wrap, mode_pad, mode_pass };
  struct {
    int dts_mode;
    int dts_conv;
    const char *filename;
    const char *ref_filename;
    size_t frame_size;
    mode_t mode;
  } tests[] = {
    { DTS_MODE_AUTO, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", fs-hs, mode_wrap },
    { DTS_MODE_AUTO, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", fs-1,  mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", fs+1,  mode_pass },

    { DTS_MODE_AUTO, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", fs-hs, mode_wrap },
    { DTS_MODE_AUTO, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", fs-1,  mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", fs+1,  mode_pass },

    { DTS_MODE_AUTO, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", fs-hs, mode_wrap },
    { DTS_MODE_AUTO, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", fs-1,  mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", fs+1,  mode_pass },

    { DTS_MODE_AUTO, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts", (fs-hs)*8/7, mode_wrap },
    { DTS_MODE_AUTO, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts", fs*8/7-1,    mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts", fs*8/7+1,    mode_pass },

    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts14", (fs-hs)*7/8, mode_wrap },
    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts14", fs*7/8-1,    mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts14", fs*7/8+1,    mode_pass },

    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", fs-hs, mode_wrap },
    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", fs-1,  mode_pad  },
    { DTS_MODE_AUTO, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", fs+1,  mode_pass },

    { DTS_MODE_WRAPPED, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", fs-hs, mode_wrap },
    { DTS_MODE_WRAPPED, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", fs-1,  mode_pass },

    { DTS_MODE_WRAPPED, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", fs-hs, mode_wrap },
    { DTS_MODE_WRAPPED, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", fs-1,  mode_pass },

    { DTS_MODE_WRAPPED, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", fs-hs, mode_wrap },
    { DTS_MODE_WRAPPED, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", fs-1,  mode_pass },

    { DTS_MODE_WRAPPED, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts", (fs-hs)*8/7, mode_wrap },
    { DTS_MODE_WRAPPED, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts", fs*8/7-1,    mode_pass },

    { DTS_MODE_WRAPPED, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts14", (fs-hs)*7/8, mode_wrap },
    { DTS_MODE_WRAPPED, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts14", fs*7/8-1,    mode_pass },

    { DTS_MODE_WRAPPED, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", fs-hs, mode_wrap },
    { DTS_MODE_WRAPPED, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", fs-1,  mode_pass },
 
    { DTS_MODE_PADDED, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", fs-1, mode_pad  },
    { DTS_MODE_PADDED, DTS_CONV_NONE, "a.dts.03f.dts", "a.dts.03f.dts", fs+1, mode_pass },

    { DTS_MODE_PADDED, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", fs-1, mode_pad  },
    { DTS_MODE_PADDED, DTS_CONV_NONE, "a.dts.03f.dts14", "a.dts.03f.dts14", fs+1, mode_pass },

    { DTS_MODE_PADDED, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", fs-1, mode_pad  },
    { DTS_MODE_PADDED, DTS_CONV_16BIT, "a.dts.03f.dts", "a.dts.03f.dts", fs+1, mode_pass },

    { DTS_MODE_PADDED, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts", fs*8/7-1, mode_pad  },
    { DTS_MODE_PADDED, DTS_CONV_16BIT, "a.dts.03f.dts14", "a.dts.03f.dts", fs*8/7+1, mode_pass },

    { DTS_MODE_PADDED, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts14", fs*7/8-1, mode_pad  },
    { DTS_MODE_PADDED, DTS_CONV_14BIT, "a.dts.03f.dts", "a.dts.03f.dts14", fs*7/8+1, mode_pass },

    { DTS_MODE_PADDED, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", fs-1, mode_pad  },
    { DTS_MODE_PADDED, DTS_CONV_14BIT, "a.dts.03f.dts14", "a.dts.03f.dts14", fs+1, mode_pass },
  };

  for (int i = 0; i < array_size(tests); i++)
  {
    // Test chain: FrameParser->DTSFrameResizer->Spdifer->[Despdifer]
    // Reference chain: FrameParser->DTSFrameResizer

    FileParser f_test;
    f_test.open_probe(tests[i].filename, &dts_header);
    BOOST_REQUIRE(f_test.is_open());

    FileParser f_ref;
    f_ref.open_probe(tests[i].ref_filename, &dts_header);
    BOOST_REQUIRE(f_ref.is_open());

    DTSFrameResize resize(tests[i].frame_size);
    SPDIFWrapper spdifer(tests[i].dts_mode, tests[i].dts_conv);
    SPDIFParser despdifer;

    FilterChain test;
    test.add_back(&resize);
    test.add_back(&spdifer);
    if (tests[i].mode != mode_pass)
      test.add_back(&despdifer);

    DTSFrameResize ref_resize(tests[i].frame_size);
    if (tests[i].mode == mode_pad)
      ref_resize.set(fs);

    compare(&f_test, &test, &f_ref, &ref_resize);
  }
}

BOOST_AUTO_TEST_SUITE_END()
