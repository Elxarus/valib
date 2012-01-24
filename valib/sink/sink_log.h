/**************************************************************************//**
  \file sink_log.h
  \brief SinkLog class
******************************************************************************/

#ifndef SINK_LOG_H
#define SINK_LOG_H

#include <vector>
#include "../sink.h"

/**************************************************************************//**
  \class LogSink
  \brief Logs incoming chunks.
******************************************************************************/

class LogSink : public SimpleSink
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

  LogSink()
  {}

  string print() const;

  /////////////////////////////////////////////////////////
  // Sink interface

  virtual bool can_open(Speakers spk_) const
  { return true; }

  virtual bool init()
  {
    log.push_back(LogEntry(entry_open, spk));
    return true;
  }

  virtual void uninit()
  {
    log.push_back(LogEntry(entry_close, spk));
  }

  virtual void reset()
  {
    log.push_back(LogEntry(entry_reset, spk));
  }

  virtual void process(const Chunk &in)
  {
    log.push_back(LogEntry(in, spk));
  }

  virtual void flush()
  {
    log.push_back(LogEntry(entry_flush, spk));
  }
};

#endif
