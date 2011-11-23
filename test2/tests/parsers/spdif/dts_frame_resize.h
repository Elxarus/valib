#ifndef TEST_DTS_FRAME_RESIZE
#define TEST_DTS_FRAME_RESIZE

#include "buffer.h"
#include "filter.h"
#include "parsers/dts/dts_header.h"

// Filter that pads DTS frames with zeros to match certain size
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
  { return frame.is_allocated() && spk.format == FORMAT_DTS; }

  bool process(Chunk &in, Chunk &out)
  {
    FrameInfo finfo;
    Chunk temp = in;
    in.clear();

    if (temp.size < dts.header_size())
      return false;
    if (!dts.parse_header(temp.rawdata, &finfo))
      return false;
    if (temp.size < finfo.frame_size || in.size > frame.size())
      return false;

    memcpy(frame.begin(), temp.rawdata, temp.size);
    memset(frame.begin() + temp.size, 0, frame.size() - temp.size);
    out.set_rawdata(frame.begin(), frame.size(), temp.sync, temp.time);
    return true;
  }

protected:
  DTSFrameParser dts;
  Rawdata frame;
};

#endif
