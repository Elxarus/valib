#include "uni_header.h"
#include "parsers\mpa\mpa_header.h"
#include "parsers\ac3\ac3_header.h"
#include "parsers\dts\dts_header.h"

const UNIHeader uni_header;

static const HeaderParser *hparsers[] =
{
  &mpa_header,
  &ac3_header,
  &dts_header
};

UNIHeader::UNIHeader()
{
  f_header_size = hparsers[0]->header_size();
  f_min_frame_size = hparsers[0]->min_frame_size();
  f_max_frame_size = hparsers[0]->max_frame_size();

  for (int i = 1; i < array_size(hparsers); i++)
  {
    if (f_header_size < hparsers[i]->header_size())
      f_header_size = hparsers[i]->header_size();

    if (f_min_frame_size < hparsers[i]->min_frame_size())
      f_min_frame_size = hparsers[i]->min_frame_size();

    if (f_max_frame_size < hparsers[i]->max_frame_size())
      f_max_frame_size = hparsers[i]->max_frame_size();
  }
}

bool
UNIHeader::can_parse(int format) const
{
  for (int i = 0; i < array_size(hparsers); i++)
   if (hparsers[i]->can_parse(format))
     return true;
  return false;
}

bool
UNIHeader::parse_header(const uint8_t *hdr, HeaderInfo *hinfo) const
{
  for (int i = 0; i < array_size(hparsers); i++)
   if (hparsers[i]->parse_header(hdr, hinfo))
     return true;
  return false;
}

bool
UNIHeader::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  for (int i = 0; i < array_size(hparsers); i++)
   if (hparsers[i]->compare_headers(hdr1, hdr2))
     return true;
  return false;
}
