/**************************************************************************//**
  \file multi_header.h
  \brief MultiFrameParser: Represents a list of header parsers as a single parser.
******************************************************************************/

#ifndef VALIB_MULTI_HEADER_H
#define VALIB_MULTI_HEADER_H

#include <vector>
#include "../parser.h"

/**************************************************************************//**
  \class MultiFrameParser
  \brief Represents a set of frame parsers as a single parser.

  A set of FrameParser objects may be represented as a single FrameParser that
  can parse multiple formats.

  MultiFrameParser provides this functionality. Given a list of parsers it
  represents this list as a single FrameParser object.

  \fn void MultiFrameParser::set_parsers(const list_t &parsers)
    \param parsers List of parsers

    Sets the list of parsers to represent.
    Null pointers are removed from the list.

  \fn void MultiFrameParser::set_parsers(FrameParser *const *parsers, size_t nparsers)
    \param parsers  Pointer to an array of parsers
    \param nparsers Number of parsers in the array

    Sets the list of parsers to represent. Useful for statically allocated list.
    Null pointers are removed from the list.

    \code
      FrameParser *parsers[] = { &ac3_frame, &dts_frame, &mpa_frame };
      MultiFrameParser multi_header(parsers, array_size(parsers));
    \endcode

  \fn void MultiFrameParser::add_parser(FrameParser *parser)
    \param parser Parser to add

    Adds a single parser to the list. Null value is ignored.

  \fn void MultiFrameParser::remove_parser(FrameParser *parser)
    \param parser Parser to remove

    Removes the parser from the list.

  \fn void MultiFrameParser::release_parsers()
    Clear the set of parsers.

  \fn list_t MultiFrameParser::get_parsers() const
    Returns the list of parsers

******************************************************************************/

class MultiFrameParser : public FrameParser
{
public:
  typedef std::vector<FrameParser *> list_t;

  MultiFrameParser();
  MultiFrameParser(const list_t &parsers);
  MultiFrameParser(FrameParser *const *parsers, size_t nparsers);

  void add_parser(FrameParser *parser);
  void remove_parser(FrameParser *parser);

  void set_parsers(const list_t &parsers);
  void set_parsers(FrameParser *const *parsers, size_t nparsers);
  void release_parsers();
  list_t get_parsers() const;

  /////////////////////////////////////////////////////////
  // FrameParser overrides

  virtual bool      can_parse(int format) const;
  virtual SyncInfo  sync_info() const;
  virtual SyncInfo  sync_info2() const;

  // Frame header operations
  virtual size_t    header_size() const;
  virtual bool      parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const;
  virtual bool      compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;

  // Frame operations
  virtual bool      first_frame(const uint8_t *frame, size_t size);
  virtual bool      next_frame(const uint8_t *frame, size_t size);
  virtual void      reset();

  virtual bool      in_sync() const;
  virtual FrameInfo frame_info() const;
  virtual string    stream_info() const;

protected:
  list_t parsers;   //!< list of parsers
  FrameParser **p;  //!< raw list of parsers
  size_t n;         //!< number of parsers

  SyncInfo sinfo;
  size_t   max_header_size;

  FrameParser *parser;

  void update();
};

#endif
