/*
  \file hresult_exception.h
  \brief Pass HRESULT parameter to exception and print it with error message.

  It is separated from other exception handling functions because it is
  Windows-specific.
*/

#ifndef HRESULT_EXCEPTION_H
#define HRESULT_EXCEPTION_H

#include <windows.h>
#include <boost/exception/all.hpp>

typedef boost::error_info<struct tag_errinfo_hresult,HRESULT > errinfo_hresult;
std::string to_string(errinfo_hresult const &e);

#endif
