/**************************************************************************//**
  \file multi_header.h
  \brief MultiHeader: Represents a list of header parsers as a single parser.
******************************************************************************/

#ifndef VALIB_MULTI_HEADER_H
#define VALIB_MULTI_HEADER_H

#include <vector>
#include "../parser.h"

/**************************************************************************//**
  \class MultiHeader
  \brief Represents a set of header parsers as a single parser.

  A set of HeaderParser objects may be represented as a single HeaderParser
  object that can parse and compare headers of any format.

  MultiHeader provides this. Given a list of parsers it represents a single
  HeaderParser object.

  \fn void MultiHeader::set_parsers(const list_t &parsers)
    \param parsers List of parsers

    Sets the list of parsers to represent.

  \fn void MultiHeader::set_parsers(const HeaderParser *const *parsers, size_t nparsers)
    \param parsers  Pointer to an array of parsers
    \param nparsers Number of parsers in the array

    Sets the list of parsers to represent. Useful for statically allocated list.

    \code
      const HeaderParser *parsers[] = { &ac3_header, &dts_header, &mpa_header };
      MultiHeader multi_header(parsers, array_size(parsers));
    \endcode

  \fn void MultiHeader::release_parsers()
    Clear the set of parsers.

******************************************************************************/

class MultiHeader : public HeaderParser
{
public:
  typedef std::vector<const HeaderParser *> list_t;

  MultiHeader();
  MultiHeader(const list_t &parsers);
  MultiHeader(const HeaderParser *const *parsers, size_t nparsers);

  void set_parsers(const list_t &parsers);
  void set_parsers(const HeaderParser *const *parsers, size_t nparsers);
  void add_parser(const HeaderParser *parser);
  void remove_parser(const HeaderParser *parser);
  void release_parsers();
  list_t get_parsers() const;

  /////////////////////////////////////////////////////////
  // HeaderParser overrides

  virtual size_t   header_size()    const { return f_header_size;    }
  virtual size_t   min_frame_size() const { return f_min_frame_size; }
  virtual size_t   max_frame_size() const { return f_max_frame_size; }
  virtual bool     can_parse(int format) const;

  virtual bool     parse_header(const uint8_t *hdr, HeaderInfo *hinfo = 0) const;
  virtual bool     compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;
  virtual string   header_info(const uint8_t *hdr) const;

protected:
  size_t f_header_size;    //!< maximum header size
  size_t f_min_frame_size; //!< minimum min_frame_size() value
  size_t f_max_frame_size; //!< maximum max_frame_size() value
  list_t parsers;          //!< list of parsers
  void update();
};

#endif
