/**************************************************************************//**
  \file exception.h
  \brief Exceptions base
******************************************************************************/

#ifndef VALIB_EXCEPTION_H
#define VALIB_EXCEPTION_H

#include <string>
#include <boost/exception/all.hpp>

#define THROW(x) BOOST_THROW_EXCEPTION(x)

/**************************************************************************//**
  \class ValibException
  \brief Base class for valib library exceptions.
******************************************************************************/

struct ValibException : virtual std::exception, virtual boost::exception {};

/**************************************************************************//**
  \class EProcessing
  \brief Processing error.

  Error during audio data flow (thrown by Filter, Source and Sink processing
  functions).

  \class EOpenSink
  \brief Sink cannot be open error.

  Format change during the data flow may require a sink to be open with a new
  format. But this operation may fail and thus exception must be thrown.

  The format causing this exception and sink name should be attached:
  \code
  if (!sink->open(spk))
    THROW(EOpenSink()
      << errinfo_spk(spk)
      << errinfo_obj_name(sink->name()));
  \endcode

  \class EOpenFilter
  \brief Filter cannot be open error.

  Format change during the data flow may require a filter to be open with a new
  format. But this operation may fail and thus exception must be thrown.

  The format causing this exception and filter name should be attached:
  \code
  if (!filter->open(spk))
    THROW(EOpenFilter()
      << errinfo_spk(spk)
      << errinfo_obj_name(filter->name()));
  \endcode

*****************************************************************************/

struct EProcessing : public ValibException {};
struct EOpenFilter : public EProcessing {};
struct EOpenSink   : public EProcessing {};


/*****************************************************************************/

#include "spk.h"

// This allows to put format into exception as follows:
// THROW(ValibException() << errinfo_spk(some_spk));
typedef boost::error_info<struct tag_errinfo_spk, Speakers > errinfo_spk;
inline std::string to_string(errinfo_spk const &e)
{ return e.value().print(); }

// This allows to put object name into exception as follows:
// THROW(ValibException() << errinfo_obj_name(filter.name()));
typedef boost::error_info<struct tag_errinfo_class_name, string > errinfo_obj_name;

#endif
