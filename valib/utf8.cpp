#include <assert.h>
#include "utf8.h"

std::wstring utf8_to_wstring(const std::string &s)
{
  std::wstring w;
  if (sizeof(wchar_t) == 2)
    utf8::utf8to16(s.begin(), s.end(), std::back_inserter(w));
  else if (sizeof(wchar_t) == 4)
    utf8::utf8to32(s.begin(), s.end(), std::back_inserter(w));
  else
    assert(false);
  return w;
}

std::string wstring_to_utf8(const std::wstring &w)
{
  std::string s;
  if (sizeof(wchar_t) == 2)
    utf8::utf16to8(w.begin(), w.end(), std::back_inserter(s));
  else if (sizeof(wchar_t) == 4)
    utf8::utf8to32(w.begin(), w.end(), std::back_inserter(s));
  else
    assert(false);
  return s;
}
