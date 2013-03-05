#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "vargs.h"

bool is_arg(const char *arg, const char *name, arg_type type)
{
  if (arg[0] != '-') return false;
  arg++;

  while (*name)
    if (*name && *arg != *name) 
      return false;
    else
      name++, arg++;

  if (type == argt_exist && *arg == '\0') return true;
  if (type == argt_bool  && (*arg == '\0' || *arg == '+' || *arg == '-')) return true;
  if (type == argt_num   && (*arg == ':' || *arg == '=')) return true;
  if (type == argt_hex   && (*arg == ':' || *arg == '=')) return true;
  if (type == argt_text  && (*arg == ':' || *arg == '=')) return true;
  if (type == argt_enum  && (*arg == ':' || *arg == '=')) return true;

  return false;
}

bool arg_bool(const char *arg)
{
  arg += strlen(arg) - 1;
  if (*arg == '-') return false;
  return true;
}

double arg_num(const char *arg)
{
  while (*arg && *arg != ':' && *arg != '=')
    arg++;
  arg++;
  return atof(arg);
}

int arg_hex(const char *arg)
{
  while (*arg && *arg != ':' && *arg != '=')
    arg++;
  arg++;

  int result = 0;
  sscanf(arg, "%x", &result);
  return result;
}

const char *arg_text(const char *arg)
{
  while (*arg && *arg != ':' && *arg != '=')
    arg++;
  arg++;
  return arg;
}

bool arg_enum(const char *arg, int &value, const enum_opt *options, size_t num_options)
{
  while (*arg && *arg != ':' && *arg != '=')
    arg++;
  arg++;

  for (size_t i = 0; i < num_options; i++)
    if (strcmp(arg, options[i].name) == 0)
    {
      value = options[i].value;
      return true;
    }

  return false;
}
