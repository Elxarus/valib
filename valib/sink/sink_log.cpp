#include <sstream>
#include "sink_log.h"

string
LogSink::LogEntry::print() const
{
  switch (type)
  {
    case entry_open:
      return string("open(") + spk.print() + ")";

    case entry_close:
      return string("close()");

    case entry_process:
    {
      std::stringstream s;
      s << "process(size = " << size;
      if (size > 0) 
        s << ", " << (rawdata? "rawdata": "linear");
      if (sync)
        s << ", sync = true, time = " << time;
      s << ")";
      return s.str();
    }

    case entry_reset:
      return string("reset()");

    case entry_flush:
      return string("flush()");
  }
  return string("???");
}

string
LogSink::print() const
{
  if (log.size() == 0)
    return string();

  string result = log[0].print();
  for (size_t i = 1; i < log.size(); i++)
    result += string("\n") + log[i].print();

  return result;
}
