/*
  Command-line arguments handling
*/

#ifndef VALIB_VARGS_H
#define VALIB_VARGS_H

struct enum_opt
{
  const char *name;
  int value;
};

enum arg_type { argt_exist, argt_bool, argt_num, argt_hex, argt_text, argt_enum };
bool is_arg(const char *arg, const char *name, arg_type type);

bool arg_bool(const char *arg);
double arg_num(const char *arg);
int arg_hex(const char *arg);
const char *arg_text(const char *arg);
bool arg_enum(const char *arg, int &value, const enum_opt *options, size_t num_options);

#endif
