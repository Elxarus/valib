/*
  Command-line arguments handling
*/

#ifndef VALIB_VARGS_H
#define VALIB_VARGS_H

#include <vector>
#include <string>

struct enum_opt
{
  const char *name;
  int value;
};

enum arg_type { argt_exist, argt_bool, argt_int, argt_double, argt_text, argt_enum };

struct arg_t
{
  struct bad_value_e
  {
    std::string arg;
    bad_value_e(const std::string &arg_): arg(arg_) {}
  };

  struct empty_arg_e {};

  std::string raw;

  arg_t()
  {}

  arg_t(const std::string &arg): raw(arg)
  {}

  arg_t(const char *arg): raw(arg)
  {}

  bool is_option(const std::string &name, arg_type type) const;

  bool as_bool() const;
  double as_double() const;
  int as_int() const;
  std::string as_text() const;

  int choose(const enum_opt *options, size_t num_options) const;
  int choose_lowcase(const enum_opt *options, size_t num_options) const;
};

typedef std::vector<arg_t> arg_list_t;
arg_list_t args_utf8(int argc, const char *argv[]);

#endif
