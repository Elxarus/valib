#include "../ac3/ac3_header.h"
#include "../dts/dts_header.h"
#include "../mpa/mpa_header.h"
#include "spdifable_header.h"

static const HeaderParser *parsers[] = 
{ &ac3_header, &dts_header, &mpa_header };

const MultiHeader spdifable_header(parsers, array_size(parsers));
