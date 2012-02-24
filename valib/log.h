/*
  Logging class
*/

#ifndef VALIB_LOG_H
#define VALIB_LOG_H

#include <stdarg.h>
#include <string>
#include "auto_file.h"
#include "vtime.h"

///////////////////////////////////////////////////////////////////////////////
// Logging is done using LogDispatcher. Listeners may connect to a dispatcher
// and receive logging events. This way we can log to any:
// * Windows debug output
// * Log file
// * Screen
// * Other (BugTrap logging)
//
// Many log sources may exist at the same time. Default log for valib is
// valib_log_dispatcher. It may be routed like any other log source. For
// example, if you like to print log to stderr, add this to begin of main():
//
// int main(int argc, conat char **argv)
// {
//   LogStderr (&valib_log_dispatcher);
//   // use valib
//   // ...
// }
// 
// By default valib log is dispatched to Windows debug output.
//
// valib_log() functions do default valib logging. Define VALIB_NO_LOG globally
// to disable logging in release builds.

struct LogEntry;
class LogDispatcher;
class LogSink;

class LogFile;
class LogStderr;
#ifdef _WIN32
class LogWindowsDebug;
#endif

///////////////////////////////////////////////////////////////////////////////
// Default log levels
// You can use other log levels, these levels are used for valib library itself.

enum levels {
  // Critical error, so program cannot continue.
  // Assertion fails and similar.
  log_critical = 0,

  // Exceptions
  log_exception = 1,

  // 'Normal' error.
  // File open, resource allocation etc.
  log_error = 2,

  // Event that require special attention.
  // Indicates possible error in code.
  log_warning = 3,

  // Event happen during execution.
  // Must appear relatively rare.
  log_event = 4,

  // Event that may appear frequently.
  // Data chunks, algorithm steps, etc.
  log_trace = 5,

  // Function call logging
  // Note, that some function calls may be considered as
  // log_event (global state changes, etc).
  log_func = 6,

  // Maximum log level
  // Guaranteed to be larger than other log levels
  log_all = 100
};

///////////////////////////////////////////////////////////////////////////////

struct LogEntry
{
  vtime_t timestamp;
  int level;
  std::string module;
  std::string message;

  LogEntry(): timestamp(local_time()), level(0)
  {}

  LogEntry(const LogEntry &other):
    timestamp(other.timestamp),
    level(other.level),
    module(other.module),
    message(other.message)
  {}

  LogEntry(vtime_t timestamp_, int level_, const std::string &module_, const std::string &message_):
    timestamp(timestamp_),
    level(level_),
    module(module_),
    message(message_)
  {}

  bool operator ==(const LogEntry &other) const
  {
    return
      timestamp == other.timestamp && 
      level == other.level &&
      module == other.module &&
      message == other.message;
  }

  std::string print() const;
};

///////////////////////////////////////////////////////////////////////////////
// LogDispatcher
// Logging functions are inline to avoid overhead of logging when not
// requested. When logging level is low the only overhead is level checking.
//
// Default log level is log_all (do not filter).

class LogDispatcher
{
public:
  LogDispatcher();
  virtual ~LogDispatcher();

  inline void set_max_log_level(int log_level);
  inline int  get_max_log_level() const;

  inline void log(const LogEntry &entry);
  inline void log(int level, const std::string &module, const std::string &message);
  inline void log(int level, const std::string &module, const char *format, ...);
  inline void vlog(int level, const std::string &module, const char *format, va_list args);
  bool is_subscribed(LogSink *sink);

protected:
  void log_impl(const LogEntry &entry);
  void vlog_impl(int level, const std::string &module, const char *format, va_list args);

  void add_sink(LogSink *sink);
  void remove_sink(LogSink *sink);
  friend class LogSink;

  int max_log_level;
  class Private;
  Private *p;
};

inline void LogDispatcher::set_max_log_level(int log_level)
{
  max_log_level = log_level;
}

inline int LogDispatcher::get_max_log_level() const
{
  return max_log_level;
}

inline void LogDispatcher::log(const LogEntry &entry)
{
  if (entry.level <= max_log_level)
    log_impl(entry);
}

inline void LogDispatcher::log(int level, const std::string &module, const std::string &message)
{
  if (level <= max_log_level)
    log_impl(LogEntry(local_time(), level, module, message));
}

inline void LogDispatcher::log(int level, const std::string &module, const char *format, ...)
{
  if (level <= max_log_level)
  {
    va_list args;
    va_start(args, format);
    vlog_impl(level, module, format, args);
    va_end(args);
  }
}

inline void LogDispatcher::vlog(int level, const std::string &module, const char *format, va_list args)
{
  if (level <= max_log_level)
    vlog_impl(level, module, format, args);
}

///////////////////////////////////////////////////////////////////////////////

class LogSink
{
public:
  LogSink(LogDispatcher *source_ = 0): max_log_level(log_all), source(0)
  { if (source_) subscribe(source_); }

  virtual ~LogSink()
  { unsubscribe(); }

  void set_max_log_level(int log_level)
  { max_log_level = log_level; }

  int get_max_log_level() const
  { return max_log_level; }

  void subscribe(LogDispatcher *new_source)
  {
    if (source == new_source) return;
    if (source) unsubscribe();
    if (new_source) new_source->add_sink(this);
    source = new_source;
  }

  void unsubscribe()
  {
    if (source) source->remove_sink(this);
    source = 0;
  }

  bool is_subscribed() const
  { return source != 0; }

  virtual void receive(const LogEntry &entry) = 0;

private:
  int max_log_level;
  LogDispatcher *source;
};

///////////////////////////////////////////////////////////////////////////////

class LogMem : public LogSink
{
public:
  LogMem(size_t max_size, LogDispatcher *source = 0);

  void resize(size_t max_size);
  size_t size() const;
  const LogEntry &operator [](size_t i) const;

  std::string log_text() const;

  virtual void receive(const LogEntry &entry);

protected:
  class Private;
  Private *p;
};

///////////////////////////////////////////////////////////////////////////////

class LogFile : public LogSink
{
public:
  LogFile(LogDispatcher *source = 0):
  LogSink(source)
  {}

  LogFile(const char *filename, LogDispatcher *source = 0):
  LogSink(source)
  { open(filename); }

  LogFile(FILE *f, bool take_ownership, LogDispatcher *source = 0):
  LogSink(source)
  { open(f, take_ownership); }

  bool open(const char *filename)
  { return file.open(filename, "w"); }

  bool open(FILE *f, bool take_ownership)
  { return file.open(f, take_ownership); }

  bool is_open() const
  { return file.is_open(); }

  void close()
  { file.close(); }

  void flush()
  { file.flush(); }

  virtual void receive(const LogEntry &entry)
  {
    if (!file.is_open())
      return;
    std::string s = entry.print() + std::string("\n");
    file.write(s.c_str(), s.size());
  }

protected:
  AutoFile file;
};

///////////////////////////////////////////////////////////////////////////////

class LogStderr : public LogFile
{
public:
  LogStderr(LogDispatcher *source = 0): LogFile(stderr, false, source)
  {}
};

///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
class LogWindowsDebug : public LogSink
{
public:
  LogWindowsDebug(LogDispatcher *source = 0):
  LogSink(source)
  {}

  virtual void receive(const LogEntry &entry);
};
#endif

///////////////////////////////////////////////////////////////////////////////

extern LogDispatcher valib_log_dispatcher;

#ifndef VALIB_NO_LOG

inline void valib_log(int level, const std::string &module, const std::string &message)
{
  if (level <= valib_log_dispatcher.get_max_log_level())
    valib_log_dispatcher.log(level, module, message);
}

inline void valib_log(int level, const std::string &module, const char *format, ...)
{
  if (level <= valib_log_dispatcher.get_max_log_level())
  {
    va_list args;
    va_start(args, format);
    valib_log_dispatcher.vlog(level, module, format, args);
    va_end(args);
  }
}

#else

inline void valib_log(int, const std::string &, const std::string &) {}
inline void valib_log(int, const std::string &, const char *, ...) {}

#endif

///////////////////////////////////////////////////////////////////////////////
// Obsolete, to be removed

#define LOG_SCREEN 1 // print log at screen
#define LOG_HEADER 2 // print timestamp header
#define LOG_STATUS 4 // show status information

#define MAX_LOG_LEVELS 128

class Log
{
protected:
  int level;
  int errors[MAX_LOG_LEVELS];
  vtime_t time[MAX_LOG_LEVELS];

  int flags;
  int istatus;
  vtime_t period;
  vtime_t tstatus;

  AutoFile f;

  void clear_status();
  void print_header(int _level);

public:
  Log(int flags = LOG_SCREEN | LOG_HEADER | LOG_STATUS, const char *log_file = 0, vtime_t period = 0.1);

  void open_group(const char *msg, ...);
  int  close_group(int expected_errors = 0);

  int     get_level();
  int     get_errors();
  vtime_t get_time();

  int     get_total_errors();
  vtime_t get_total_time();

  void status(const char *msg, ...);
  void msg(const char *msg, ...);
  int  err(const char *msg, ...);
  int  err_close(const char *msg, ...);
};

#endif
