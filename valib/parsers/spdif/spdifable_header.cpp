#include "../ac3/ac3_header.h"
#include "../dts/dts_header.h"
#include "../eac3/eac3_header.h"
#include "../mpa/mpa_header.h"
#include "../multi_header.h"
#include "spdifable_header.h"


static const HeaderParser *parsers[] = 
{ &ac3_header, &eac3_header, &dts_header, &mpa_header };

const HeaderParser *spdifable_header()
{
  static const MultiHeader header(parsers, array_size(parsers));
  return &header;
};
