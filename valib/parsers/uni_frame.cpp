#include "uni_frame.h"
#include "uni_header.h"


UNIFrame::UNIFrame()
{
  reset();
}

/////////////////////////////////////////////////////////
// FrameParser overrides

const HeaderParser *
UNIFrame::header_parser()
{
  return &uni_header;
}

void
UNIFrame::reset()
{
  spk = spk_unknown;
  nsamples = 0;
  samples.zero();

  parser = 0;

  mpa.reset();
  ac3.reset();
  dts.reset();
}

bool
UNIFrame::parse_frame(uint8_t *frame, size_t size)
{
  HeaderInfo hinfo;
  if (!uni_header.parse_header(frame, &hinfo))
    return false;

  switch (hinfo.spk.format)
  {
    case FORMAT_MPA: parser = &mpa; break;
    case FORMAT_AC3: parser = &ac3; break;
    case FORMAT_DTS: parser = &dts; break;
    default:
      parser = 0;
      return false;
  }

  if (parser->parse_frame(frame, size))
  {
    spk = parser->get_spk();
    samples = parser->get_samples();
    nsamples = parser->get_nsamples();
    return true;
  }
  else
    return false;
}

size_t
UNIFrame::stream_info(char *buf, size_t size) const
{
  if (parser)
    return parser->stream_info(buf, size);

  if (buf && size) buf[0] = 0;
  return 0;
}

size_t
UNIFrame::frame_info(char *buf, size_t size) const
{
  if (parser)
    return parser->frame_info(buf, size);

  if (buf && size) buf[0] = 0;
  return 0;
}
