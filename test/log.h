#ifndef LOG_H
#define LOG_H

#define MAX_LOG_LEVELS 128

#include <stdarg.h>

class Log
{
protected:
  int level;
  int errors[MAX_LOG_LEVELS];

public:
  Log()
  {
    level = 0;
    errors[level] = 0;
  };

  virtual int open_group(const char *msg, ...)
  {
    if (level < MAX_LOG_LEVELS)
    {
      level++;
      errors[level] = 0;
    }
    return level;
  }

  virtual int close_group()
  {
    if (level)
    {
      errors[level-1] += errors[level];
      level--;
    }
    return level;
  }

  virtual int get_level() 
  { 
    return level; 
  }

  virtual int get_total_errors()
  { 
    int result = 0;
    for (int i = 0; i <= level; i++)
      result += errors[level];

    return result;
  }

  virtual int get_group_errors()
  {
    return errors[level]; 
  }

  virtual void msg(const char *msg, ...) = 0;
  virtual void err(const char *msg, ...) = 0;
};

class ScreenLog : public Log
{
  inline print_header()
  {
    // todo: timestamp
    // indent
    int i = level;
    while (i--)
      printf("  ");
  }

public: 
  int open_group(const char *_msg, ...)
  {
    print_header();
    printf("> ", _msg);
    va_list list;
    va_start(list, _msg);
    vprintf(_msg, list);
    va_end(list);
    printf("\n");
    return Log::open_group(_msg);
  }

  int close_group()
  {
    print_header();
    if (!errors[level])
      printf("< Ok\n");
    else
      printf("< Errors: %i\n",  errors[level]);

    return Log::close_group();
  }

  void msg(const char *_msg, ...)
  {
    print_header();
    printf("* ");
    va_list list;
    va_start(list, _msg);
    vprintf(_msg, list);
    va_end(list);
    printf("\n");
  }

  void err(const char *_msg, ...)
  {
    errors[level]++;

    print_header();
    printf("! error: ");
    va_list list;
    va_start(list, _msg);
    vprintf(_msg, list);
    va_end(list);
    printf("\n");
  }
};

#endif