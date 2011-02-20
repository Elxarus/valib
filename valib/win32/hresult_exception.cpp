#include <sstream>
#include "hresult_exception.h"

std::string to_string(errinfo_hresult const &e)
{
  HRESULT hresult = e.value();
  LPSTR error_text = 0;

  std::ostringstream tmp;
  tmp << std::hex << "0x" << hresult;

  FormatMessageA( 
    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
    FORMAT_MESSAGE_FROM_SYSTEM | 
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    hresult,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    error_text,
    0,
    NULL 
  );

  if (error_text)
  {
    tmp << " " << error_text;
    LocalFree(error_text);
  }

  return tmp.str();
}
