#ifndef LOG_H
#define LOG_H

#define MAX_LOG_LEVELS 128

#include <stdio.h>
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

  virtual void open_group(const char *msg, ...)
  {
    if (level < MAX_LOG_LEVELS)
    {
      level++;
      errors[level] = 0;
    }
  }

  virtual int close_group()
  {
    if (level)
    {
      errors[level-1] += errors[level];
      level--;
      return errors[level+1];
    }
    else
      return errors[0];
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

  virtual void status(const char *msg, ...) = 0;
  virtual void msg(const char *msg, ...) = 0;
  virtual int  err(const char *msg, ...) = 0;
};

static const char *statuses[] = { "* ", "- ", "\\ ", "| ", "/ " };
class ScreenLog : public Log
{
protected:
  char current_status;
  inline print_header(int _level)
  {
    // erase status line
    if (current_status)
    {
      printf("                                                                               \n");
      current_status = 0;
    }

    // todo: timestamp
    // indent
    while (_level--)
      printf("  ");
  }

public:
  ScreenLog()
  {
    current_status = 0;
  }

  void open_group(const char *_msg, ...)
  {
    print_header(level);
    printf("> ", _msg);
    va_list list;
    va_start(list, _msg);
    vprintf(_msg, list);
    va_end(list);
    printf("\n");
    Log::open_group(_msg);
  }

  int close_group()
  {
    if (level)
      print_header(level-1);
    else
      print_header(0);

    if (!errors[level])
      printf("< Ok\n");
    else
      printf("< Errors: %i\n",  errors[level]);

    return Log::close_group();
  }

  void status(const char *_msg, ...)
  {
    current_status++;
    if (current_status >= (sizeof(statuses) / sizeof(statuses[0])))
      current_status = 1;
    printf(statuses[current_status]);

    va_list list;
    va_start(list, _msg);
    vprintf(_msg, list);
    va_end(list);
    printf("\r");
  }

  void msg(const char *_msg, ...)
  {
    print_header(level);
    printf("* ");
    va_list list;
    va_start(list, _msg);
    vprintf(_msg, list);
    va_end(list);
    printf("\n");
  }

  int err(const char *_msg, ...)
  {
    print_header(level);
    printf("! error: ");
    va_list list;
    va_start(list, _msg);
    vprintf(_msg, list);
    va_end(list);
    printf("\n");

    errors[level]++;
    return errors[level];
  }
};

#endif