#ifndef UTILS_H
#define UTILS_H

#include "log.h"
#include "filter.h"

// Get data from source stream process it using given filter and compare output with 
// reference stream. Filter should accept source stream format. If reference stream 
// has FORMAT_UNKNOWN then output format is not tested and data is compared binary 
// (so output should be raw in this case, not FORMAT_LINEAR).

int compare(Log *log, Source *src, Filter *src_filter, Source *ref, Filter *ref_filter = 0);

// Same as compare(), but files are used instead of sources.
// Input format of filter should be configured!

int compare_file(Log *log, Speakers spk_src, const char *fn_src, Filter *src_filter, const char *fn_ref);

// Crash test (process 1Mb of noise)

int crash_test(Log *log, Speakers spk, Filter *filter);

#endif
