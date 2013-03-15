#ifndef VALIB_UTF8_H
#define VALIB_UTF8_H

#include <string>
#include "../3rdparty/utf8/utf8.h"

std::wstring utf8_to_wstring(const std::string &s);
std::string wstring_to_utf8(const std::wstring &w);

#endif
