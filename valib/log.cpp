#include <algorithm>
#include <vector>
#include <time.h>
#include "win32/thread.h"
#include "log.h"

using std::string;

// Default message buffer size for sprintf.
// Buffer grows if message exceeds this limit, but this process is not effective.
// See LogDispatcher::vlog()
static const size_t def_message_size(256);

// Max message buffer size for sprintf
// See LogDispatcher::vlog()
static const size_t max_message_size = 16384;

///////////////////////////////////////////////////////////////////////////////

LogDispatcher valib_log_dispatcher;
#if defined(_WIN32) && defined(_DEBUG)
static LogWindowsDebug windows_debug(&valib_log_dispatcher, log_event);
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

  return string(buf, len) + module + string(": ") + message;
}

///////////////////////////////////////////////////////////////////////////////
// LogDispatcher

class LogDispatcher::Private
{
public:
  std::vector<LogSink *> sinks;
  CritSec sink_lock;
};

LogDispatcher::LogDispatcher(): max_log_level(log_trace), p(new LogDispatcher::Private())
{}

LogDispatcher::~LogDispatcher()
{
  // Unsubscribe all listeners
  // Do not lock here! Critical section is to be deleted.
  // If destructor needs locking it's a bug of its lifetime.
  std::vector<LogSink *> copy = p->sinks;
  for (size_t i = 0; i < copy.size(); i++)
    copy[i]->unsubscribe();
  delete p;
}

void LogDispatcher::log_impl(const LogEntry &entry)
{
  AutoLock lock(&p->sink_lock);
  for (size_t i = 0; i < p->sinks.size(); i++)
    if (entry.level <= p->sinks[i]->get_max_log_level())
      p->sinks[i]->receive(entry);
}

void LogDispatcher::vlog_impl(int level, const std::string &module, const char *format, va_list args)
{
  AutoLock lock(&p->sink_lock);

  // Allocate message buffer only once
  // Expand it only when required up to max_message_size.
  static std::vector<char> buf(def_message_size);

  size_t len = 0;
  while (true)
  {
    len = vsnprintf(&(buf[0]), buf.size(), format, args);
    if (len < buf.size() || buf.size() >= max_message_size)
      break;
    buf.resize(buf.size() * 2);
  }

  // Drop newline from the end of the line
  while (len && (buf[len-1] == '\n' || buf[len-1] == '\r' || buf[len-1] == 0))
    len--;

  // Do not include trailing zero! (test will fail)
  log(LogEntry(local_time(), level, module, string(&buf.front(), len)));
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
// LogMem

class LogMem::Private
{
public:
  LogMem::Private(): pos(0), max_size(0)
  {}

  size_t pos;
  size_t max_size;
  std::vector<LogEntry> entries;
};

LogMem::LogMem(size_t max_size, LogDispatcher *source, int log_level): LogSink(source, log_level), p(new LogMem::Private)
{
  p->max_size = max_size;
}

LogMem::~LogMem()
{
  delete p;
}

void LogMem::resize(size_t max_size)
{
  size_t i, j;
  size_t size = MIN(max_size, p->entries.size());
  std::vector<LogEntry> entries(size);

  if (max_size < p->entries.size())
    p->pos += p->entries.size() - max_size;

  for (i = 0, j = p->pos; i < size && j < p->entries.size(); i++, j++)
    entries[i] = p->entries[j];
  for (j = 0; i < size && j < p->pos; i++, j++)
    entries[i] = p->entries[j];

  p->pos = 0;
  p->max_size = max_size;
  p->entries = entries;
}

size_t LogMem::size() const
{
  return p->entries.size();
}

const LogEntry &LogMem::operator [](size_t i) const
{
  i += p->pos;
  while (i >= p->entries.size())
    i -= p->entries.size();
  return p->entries[i];
}

string LogMem::log_text() const
{
  size_t i;
  string s;
  for (i = p->pos; i < p->entries.size(); i++)
    s += p->entries[i].print() + nl;
  for (i = 0; i < p->pos; i++)
    s += p->entries[i].print() + nl;
  return s;
}

void LogMem::receive(const LogEntry &entry)
{
  if (p->entries.size() < p->max_size)
    p->entries.push_back(entry);
  else if (p->max_size != 0)
  {
    p->entries[p->pos++] = entry;
    if (p->pos >= p->entries.size())
      p->pos = 0;
  }
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
