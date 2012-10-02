/**************************************************************************//**
  \file log_filter.h
  \brief LogFilter class
******************************************************************************/

#ifndef LOG_FILTER_H
#define LOG_FILTER_H

#include <vector>
#include "passthrough.h"

/**************************************************************************//**
  \class LogFilter
  \brief Logs incoming chunks for debugging/testing.
******************************************************************************/

class LogFilter : public Passthrough
{
public:
  enum entry_type_t {
    entry_open,
    entry_close,
    entry_process,
    entry_reset,
    entry_flush
  };

  struct LogEntry
  {
    entry_type_t type;

    Speakers spk;

    size_t  size;
    bool    rawdata;

    bool    sync;
    vtime_t time;

    LogEntry(entry_type_t type_, Speakers spk_):
      type(type_), spk(spk_),
      size(0), rawdata(true),
      sync(false), time(0)
    {}

    LogEntry(const Chunk &chunk, Speakers spk_):
      type(entry_process), spk(spk_),
      size(chunk.size), rawdata(chunk.rawdata != 0),
      sync(chunk.sync), time(chunk.time)
    {}

    string print() const;
  };

  std::vector<LogEntry> log;

  LogFilter()
  {}

  string print() const;

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual bool open(Speakers spk)
  {
    log.push_back(LogEntry(entry_open, spk));
    return Passthrough::open(spk);
  }

  virtual void uninit()
  {
    log.push_back(LogEntry(entry_close, spk));
  }

  virtual void reset()
  {
    log.push_back(LogEntry(entry_reset, spk));
  }

  virtual bool process(Chunk &in, Chunk &out)
  {
    log.push_back(LogEntry(in, spk));
    return Passthrough::process(in, out);
  }

  virtual bool flush(Chunk &out)
  {
    log.push_back(LogEntry(entry_flush, spk));
    return Passthrough::flush(out);
  }
};

#endif
