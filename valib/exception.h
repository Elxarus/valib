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
******************************************************************************/

struct EProcessing : public ValibException {};

/*****************************************************************************/

#include "spk.h"

typedef boost::error_info<struct tag_errinfo_hresult,Speakers > errinfo_spk;
inline std::string to_string(errinfo_spk const &e)
{ return e.value().print(); }

#endif
