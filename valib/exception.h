/**************************************************************************//**
  \file exception.h
  \brief Exceptions base
******************************************************************************/

#ifndef VALIB_EXCEPTION_H
#define VALIB_EXCEPTION_H

#include <string>

using std::string;

/**************************************************************************//**
  \class ValibException
  \brief Base class for valib library exceptions.
******************************************************************************/

class ValibException
{
public:
  string source; //!< Error source (class name)
  int code;      //!< Error code
  string text;   //!< Error text

  ValibException(string source_, int code_, string text_):
  source(source_), code(code_), text(text_)
  {}

  ValibException(string source_, string text_):
  source(source_), code(-1), text(text_)
  {}
};

/**************************************************************************//**
  \class EProcessing
  \brief Processing error.

  Error during audio data flow (thrown by Filter, Source and Sink processing
  functions).
******************************************************************************/

class EProcessing : public ValibException
{
public:
  EProcessing(string source_, int code_, string text_):
  ValibException(source_, code_, text_)
  {}

  EProcessing(string source_, string text_):
  ValibException(source_, text_)
  {}
};

#endif
