#ifndef LOG_H
#define LOG_H

#define MAX_LOG_LEVELS 128

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "vtime.h"

class Log
{
protected:
  int level;
  int errors[MAX_LOG_LEVELS];
  vtime_t time[MAX_LOG_LEVELS];

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
      time[level] = local_time();
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
    // erase status line (if it is)
    if (current_status)
    {
      fprintf(stderr, "                                                                               \r");
      current_status = 0;
    }

    // timestamp
    time_t t = (time_t)local_time();
    tm *pt = gmtime(&t);
    if (pt)
      printf("%04i/%02i/%02i %02i:%02i:%02i | ", pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday, pt->tm_hour, pt->tm_min, pt->tm_sec);
    else
      printf("0000/00/00 00:00:00 | ");

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

    vtime_t elapsed = local_time() - time[level];

    if (!errors[level])
      printf("< Ok (%i:%02i)\n", int(elapsed) / 60, int(elapsed) % 60);
    else
      printf("< Errors: %i (%i:%02i)\n", errors[level], int(elapsed) / 60, int(elapsed) % 60);

    return Log::close_group();
  }

  void status(const char *_msg, ...)
  {
    current_status++;
    if (current_status >= (sizeof(statuses) / sizeof(statuses[0])))
      current_status = 1;
    fprintf(stderr, statuses[current_status]);

    vtime_t elapsed = local_time() - time[level];
    fprintf(stderr, "%i:%02i ", int(elapsed) / 60, int(elapsed) % 60);

    va_list list;
    va_start(list, _msg);
    vfprintf(stderr, _msg, list);
    va_end(list);
    fprintf(stderr, "\r");
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