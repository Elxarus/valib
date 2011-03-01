#ifndef SUITE_H
#define SUITE_H

#include "filter.h"
#include "source.h"

///////////////////////////////////////////////////////////////////////////////
// Compare funcitons
///////////////////////////////////////////////////////////////////////////////

// Compare two streams from two sources. Stream formats are not compared when
// reference stream has FORMAT_RAWDATA format. In this case data is compared
// binary, so formats of both streams must be raw, not FORMAT_LINEAR.
// Second form of compare() uses filters to process streams.

void compare(Source *src, Source *ref);
void compare(Source *src, Filter *src_filter, Source *ref, Filter *ref_filter = 0);

// Check number of stream changes and total number of chunks (optionally)

void check_streams_chunks(Source *src, int streams, int chunks = -1);
void check_streams_chunks(Source *src, Filter *f, int streams, int chuns = -1);

sample_t calc_peak(Source *s);
sample_t calc_peak(Source *s, Filter *f);

double calc_rms(Source *s);
double calc_rms(Source *s, Filter *f);

sample_t calc_diff(Source *s1, Source *s2);
sample_t calc_diff(Source *s1, Filter *f1, Source *s2, Filter *f2);

double calc_rms_diff(Source *s1, Source *s2);
double calc_rms_diff(Source *s1, Filter *f1, Source *s2, Filter *f2);

#endif
