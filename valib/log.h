/*
  Logging class
*/

#ifndef VALIB_LOG_H
#define VALIB_LOG_H

#include <vector>
#include <string>
#include "auto_file.h"
#include "vtime.h"
#include "win32/thread.h"

///////////////////////////////////////////////////////////////////////////////

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

  // 'Normal' error.
  // File open, resource allocation etc.
  log_error = 1,

  // Event that require special attention.
  // Indicates possible error in code.
  log_warning = 2,

  // Event happen during execution.
  // Must appear relatively rare.
  log_event = 3,

  // Event that may appear frequently.
  // Data chunks, algorithm steps, etc.
  log_trace = 4,

  // Function call enter/exit.
  // Note, that some function calls may be considered as
  // log_event (global state changes, etc).
  log_function_call = 6
};

///////////////////////////////////////////////////////////////////////////////

struct LogEntry
{
  vtime_t timestamp;
  int level;
  std::string message;

  LogEntry(): timestamp(local_time()), level(0)
  {}

  LogEntry(const LogEntry &other):
  timestamp(other.timestamp), level(other.level), message(other.message)
  {}

  LogEntry(vtime_t timestamp_, int level_, const std::string &message_):
  timestamp(timestamp_), level(level_), message(message_)
  {}

  LogEntry(int level_, const std::string &message_):
  timestamp(local_time()), level(level_), message(message_)
  {}

  bool operator ==(const LogEntry &other) const
  {
    return
      timestamp == other.timestamp && 
      level == other.level &&
      message == other.message;
  }

  std::string print() const;
};

///////////////////////////////////////////////////////////////////////////////

class LogDispatcher
{
public:
  LogDispatcher()
  {}

  ~LogDispatcher()
  {}

  void log(const LogEntry &entry);
  void log(int level, std::string message);
  void log(int level, const char *format, ...);
  void vlog(int level, const char *format, va_list args);
  bool is_subscribed(LogSink *sink);

protected:
  void add_sink(LogSink *sink);
  void remove_sink(LogSink *sink);
  friend class LogSink;

  CritSec sink_lock;
  std::vector<LogSink *> sinks;
};

///////////////////////////////////////////////////////////////////////////////

class LogSink
{
public:
  LogSink(LogDispatcher *source_ = 0): source(0)
  { if (source_) subscribe(source_); }

  virtual ~LogSink()
  { unsubscribe(); }

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

  virtual void receive(const LogEntry &entry) = 0;

private:
  LogDispatcher *source;
};

///////////////////////////////////////////////////////////////////////////////

class LogFile : public LogSink
{
public:
  LogFile()
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

inline void valib_log(int level, std::string message)
{ valib_log_dispatcher.log(level, message); }

inline void valib_log(int level, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  valib_log_dispatcher.log(level, format, args);
  va_end(args);
}

#else

void valib_log(int level, std::string message) {}
void valib_log(int level, const char *message) {}
void valib_log(int level, const char *format, ...) {}

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
