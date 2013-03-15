#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <boost/algorithm/string.hpp>    
#include <boost/lexical_cast.hpp>
#include "utf8.h"
#include "defs.h"
#include "vargs.h"

using std::string;

struct
{
  const char *str;
  bool value;
} bool_strings[] =
{
  { "+", true },
  { "1", true },
  { "y", true },
  { "yes", true },
  { "true", true },
  { "-", false },
  { "0", false },
  { "n", false },
  { "no", false },
  { "false", false },
};

bool
arg_t::is_option(const string &name, arg_type type) const
{
  if (raw.size() < name.size() + 1) return false;
  size_t value_len = raw.size() - name.size() - 1;

  if (raw[0] != '-')
    return false;

  if (raw.compare(1, name.size(), name) != 0)
    return false;

  string::const_iterator pos = raw.begin() + name.size() + 1;

  switch (type)
  {
  case argt_exist:
    if (value_len == 0) return true;
    return false;

  case argt_bool:
    if (value_len == 0) return true;
    if (value_len == 1 && (*pos == '+' || *pos == '-')) return true;
    if (value_len >= 1 && (*pos == ':' || *pos == '='))
      return true;
    return false;

  case argt_int:
  case argt_double:
  case argt_text:
  case argt_enum:
    if (value_len == 0) return false;
    if (*pos != ':' && *pos != '=') return false;
    return true;
  }

  return false;
};

bool
arg_t::as_bool() const
{
  if (raw.empty()) throw empty_arg_e();
  size_t pos = raw.find_first_of(":=");
  if (pos != string::npos)
  {
    string value = boost::algorithm::to_lower_copy(raw.substr(pos + 1));
    for (size_t i = 0; i < array_size(bool_strings); i++)
      if (value == bool_strings[i].str)
        return bool_strings[i].value;
    throw bad_value_e(raw);
  }
  else
  {
    char last = *raw.rbegin();
    if (last == '+') return true;
    if (last == '-') return false;
    return true;
  }
}

// see http://stackoverflow.com/questions/1070497/c-convert-hex-string-to-signed-integer
template <class ElemT>
struct HexTo {
  ElemT value;
  operator ElemT() const { return value; }
  friend std::istream& operator>>(std::istream& in, HexTo& out)
  {
    in >> std::hex >> out.value;
    return in;
  }
};

int
arg_t::as_int() const
{
  string value = as_text();
  try {
    if (value.size() > 2 && value[0] == '0' && value[1] == 'x')
      return boost::lexical_cast<HexTo<int>>(value.substr(2));
    return boost::lexical_cast<int>(value);
  }
  catch (boost::bad_lexical_cast) {
    throw bad_value_e(raw);
  }
}

double
arg_t::as_double() const
{
  string value = as_text();
  try {
    return boost::lexical_cast<double>(value);
  }
  catch (boost::bad_lexical_cast) {
    throw bad_value_e(raw);
  }
}

string
arg_t::as_text() const
{
  if (raw.empty()) throw empty_arg_e();
  size_t pos = raw.find_first_of(":=");
  if (pos == string::npos) throw bad_value_e(raw);
  return raw.substr(pos + 1);
}

int
arg_t::choose(const enum_opt *options, size_t num_options) const
{
  string value = as_text();
  for (size_t i = 0; i < num_options; i++)
    if (value == options[i].name)
      return options[i].value;
  throw bad_value_e(raw);
}

int
arg_t::choose_lowcase(const enum_opt *options, size_t num_options) const
{
  string value = boost::algorithm::to_lower_copy(as_text());
  for (size_t i = 0; i < num_options; i++)
    if (value == options[i].name)
      return options[i].value;
  throw bad_value_e(raw);
}

///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include "windows.h"
#endif

arg_list_t args_utf8(int argc, const char *argv[])
{
  arg_list_t args;

# ifdef _WIN32
    int wargc = 0;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    if (wargv != NULL)
    {
      for (int i = 0; i < wargc; i++)
        args.push_back(wstring_to_utf8(wargv[i]));
      LocalFree(wargv);
      return args;
    }
#endif

  // Default implementation: just pass all arguments as is.
  // In most *nix systems arguments are already in UTF-8
  for (int i = 0; i < argc; i++)
    args.push_back(argv[i]);
  return args;
}
