#include <algorithm>
#include <vector>
#include <stdarg.h>
#include <time.h>
#include "win32/thread.h"
#include "log.h"

using std::string;

// Default message buffer size for sprintf.
// Buffer grows if message exceeds this limit, but this process is not effective.
// See LogDispatcher::vlog()
static const size_t def_message_size(256);

///////////////////////////////////////////////////////////////////////////////

LogDispatcher valib_log_dispatcher;
#ifdef _WIN32
static LogWindowsDebug windows_debug(&valib_log_dispatcher);
#endif

///////////////////////////////////////////////////////////////////////////////
// LogEntry

string
LogEntry::print() const
{
  // timestamp
  time_t t = (time_t)timestamp;
  tm *pt = gmtime(&t);

  // On 64bit system max int value length is 19 chars + 1 char sign.
  // Other string is 25 chars + 1 char trailing zero.
  // 46 chars total. 64 just in case...
  char buf[64] = "0000-00-00 00:00:00 | 0 | ";
  size_t len = 0;
  if (pt)
    len = _snprintf(buf, array_size(buf), "%04i-%02i-%02i %02i:%02i:%02i | %i | ",
      pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday, 
      pt->tm_hour, pt->tm_min, pt->tm_sec, 
      level);

  return string(buf, len) + message;
}

///////////////////////////////////////////////////////////////////////////////
// LogDispatcher

class LogDispatcher::Private
{
public:
  std::vector<LogSink *> sinks;
  CritSec sink_lock;
};

LogDispatcher::LogDispatcher(): p(new LogDispatcher::Private())
{}

LogDispatcher::~LogDispatcher()
{
  delete p;
}

void LogDispatcher::log(const LogEntry &entry)
{
  AutoLock lock(&p->sink_lock);
  for (size_t i = 0; i < p->sinks.size(); i++)
    p->sinks[i]->receive(entry);
}

void LogDispatcher::log(int level, const std::string &message)
{
  AutoLock lock(&p->sink_lock);
  log(LogEntry(level, message));
}

void LogDispatcher::log(int level, const char *format, ...)
{
  AutoLock lock(&p->sink_lock);
  va_list args;
  va_start(args, format);
  vlog(level, format, args);
  va_end(args);
}

void LogDispatcher::vlog(int level, const char *format, va_list args)
{
  AutoLock lock(&p->sink_lock);
  std::vector<char> buf(def_message_size);
  size_t len = 0;
  while (true)
  {
    len = vsnprintf(&(buf[0]), buf.size(), format, args);
    if (len < buf.size())
      break;
    buf.resize(buf.size() * 2);
  }
  buf.resize(len); // do not include trailing zero!
  log(LogEntry(level, string(buf.begin(), buf.end())));
}

bool LogDispatcher::is_subscribed(LogSink *sink)
{
  AutoLock lock(&p->sink_lock);
  return std::find(p->sinks.begin(), p->sinks.end(), sink) != p->sinks.end();
}

void LogDispatcher::add_sink(LogSink *sink)
{
  AutoLock lock(&p->sink_lock);
  if (sink && std::find(p->sinks.begin(), p->sinks.end(), sink) == p->sinks.end())
    p->sinks.push_back(sink);
}

void LogDispatcher::remove_sink(LogSink *sink)
{
  AutoLock lock(&p->sink_lock);
  p->sinks.erase(
    std::remove(p->sinks.begin(), p->sinks.end(), sink),
    p->sinks.end());
}

///////////////////////////////////////////////////////////////////////////////
// LogWindowsDebug

#ifdef _WIN32
void LogWindowsDebug::receive(const LogEntry &entry)
{
  OutputDebugString(entry.print().c_str());
  OutputDebugString("\r\n");
}
#endif

///////////////////////////////////////////////////////////////////////////////

#ifndef VALIB_NO_LOG
void valib_log(int level, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  valib_log_dispatcher.vlog(level, format, args);
  va_end(args);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// Obsolete

static const char *statuses[] = { "* ", "- ", "\\ ", "| ", "/ " };

inline void log_print(int flags, AutoFile &f, const char *format, va_list list)
{
  if (flags & LOG_SCREEN)
    vprintf(format, list);
  if (f.is_open())
  {
    vfprintf(f, format, list);
    fflush(f);
  }
}

inline void log_print(int flags, AutoFile &f, const char *format, ...)
{
  va_list list;
  va_start(list, format);
  log_print(flags, f, format, list);
  va_end(list);
}


Log::Log(int _flags, const char *_log_file, vtime_t _period)
{
  level = 0;
  errors[0] = 0;
  time[0] = local_time();
 
  flags = _flags;
  istatus = 0;
  period = _period;
  tstatus = local_time();

  if (_log_file)
    if (!f.open(_log_file, "w"))
      msg("Cannot open log file %s", _log_file);
}

void
Log::clear_status()
{
  // erase status line (if it is)
  if (istatus)
  {
    printf("                                                                               \r");
    istatus = 0;
  }
}

void 
Log::print_header(int _level)
{
  clear_status();

  if (flags & LOG_HEADER)
  {
    // timestamp
    time_t t = (time_t)local_time();
    tm *pt = gmtime(&t);
    if (pt)
      log_print(flags, f, "%04i/%02i/%02i %02i:%02i:%02i | ", 
          pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday, 
          pt->tm_hour, pt->tm_min, pt->tm_sec);
    else
      log_print(flags, f, "0000/00/00 00:00:00 | ");
  }
  
  // indent
  while (_level--)
    log_print(flags, f, "  ");
}



void 
Log::open_group(const char *msg, ...)
{
  print_header(level);

  log_print(flags, f, "> ");
  va_list list;
  va_start(list, msg);
  log_print(flags, f, msg, list);
  va_end(list);
  log_print(flags, f, "\n");

  if (level < MAX_LOG_LEVELS)
  {
    level++;
    errors[level] = 0;
    time[level] = local_time();
  }
}

int 
Log::close_group(int _expected_errors)
{
  print_header(level? level-1: 0);

  vtime_t elapsed = local_time() - time[level];

  if (!errors[level] && !_expected_errors)
    log_print(flags, f, "< Ok. (%i:%02i)\n", 
      int(elapsed) / 60, int(elapsed) % 60);
  else if (errors[level] == _expected_errors)
    log_print(flags, f, "< Ok. Expected errors: %i (%i:%02i)\n", 
      _expected_errors, int(elapsed) / 60, int(elapsed) % 60);
  else if (!_expected_errors)
    log_print(flags, f, "< Fail. Errors: %i (%i:%02i)\n", 
      errors[level], int(elapsed) / 60, int(elapsed) % 60);
  else
    log_print(flags, f, "< Fail. Errors/expected errors: %i/%i (%i:%02i)\n", 
      errors[level], _expected_errors, int(elapsed) / 60, int(elapsed) % 60);

  if (errors[level] > _expected_errors)
    errors[level] = errors[level] - _expected_errors;
  else
    errors[level] = _expected_errors - errors[level];

  if (level)
  {
    errors[level-1] += errors[level];
    level--;
    return errors[level+1];
  }
  else
    return errors[0];
}

int
Log::get_level() 
{ 
  return level; 
}

int 
Log::get_errors()
{
  return errors[level]; 
}

vtime_t
Log::get_time()
{
  return local_time() - time[level];
}

int 
Log::get_total_errors()
{ 
  int result = 0;
  for (int i = 0; i <= level; i++)
    result += errors[level];

  return result;
}

vtime_t
Log::get_total_time()
{
  return local_time() - time[0];
}


void 
Log::status(const char *_msg, ...)
{
  if (flags & LOG_STATUS)
  {
    vtime_t t = local_time();
    if (t > tstatus + period)
    {
      tstatus = t;
      istatus++;
      if (istatus >= (sizeof(statuses) / sizeof(statuses[0])))
        istatus = 1;
      fprintf(stderr, statuses[istatus]);
  
      va_list list;
      va_start(list, _msg);
      vfprintf(stderr, _msg, list);
      va_end(list);
      fprintf(stderr, "\r");
    }
  }
}

void 
Log::msg(const char *_msg, ...)
{
  print_header(level);

  log_print(flags, f, "* ");
  va_list list;
  va_start(list, _msg);
  log_print(flags, f, _msg, list);
  va_end(list);
  log_print(flags, f, "\n");
}

int 
Log::err(const char *_msg, ...)
{
  print_header(level);

  log_print(flags, f, "! error: ");
  va_list list;
  va_start(list, _msg);
  log_print(flags, f, _msg, list);
  va_end(list);
  log_print(flags, f, "\n");

  errors[level]++;
  return errors[level];
}

int 
Log::err_close(const char *_msg, ...)
{
  print_header(level);

  log_print(flags, f, "! error: ");
  va_list list;
  va_start(list, _msg);
  log_print(flags, f, _msg, list);
  va_end(list);
  log_print(flags, f, "\n");

  errors[level]++;
  return close_group();
}
