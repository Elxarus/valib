/*
  Universal header parser
*/

#ifndef UNI_HEADER_H
#define UNI_HEADER_H

#include "parser.h"

class UNIHeader : public HeaderParser
{
protected:
  size_t f_header_size;
  size_t f_min_frame_size;
  size_t f_max_frame_size;

public:
  UNIHeader();

  /////////////////////////////////////////////////////////
  // HeaderParser overrides

  virtual size_t   header_size()    const { return f_header_size;    }
  virtual size_t   min_frame_size() const { return f_min_frame_size; }
  virtual size_t   max_frame_size() const { return f_max_frame_size; }
  virtual bool     can_parse(int format) const;

  virtual bool     parse_header(const uint8_t *hdr, HeaderInfo *hinfo = 0) const;
  virtual bool     compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;
};

extern const UNIHeader uni_header;

#endif
