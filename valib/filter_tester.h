/*
  FilterTester class

  This class is proposed to test correctness of calls to filter interface and
  check that format change rules are fulfilled (see Format change rules).

  This class is a filter wrapper and may be used anywhere instead of the real
  filter. So if some filter has a strange behaviour we may wrap it and check
  for possible problems of filter usage.

  Filter must exist during the tester lifetime. 
  Filter is not destroyed with the tester.
*/

#ifndef FILTER_TESTER_H
#define FILTER_TESTER_H

#include "filter.h"
#include "log.h"

class FilterTester : public Filter
{
protected:
  Filter *filter;
  Log    *log;

  Speakers spk_input;  // input format
  bool     ofdd;       // output format is data-dependent
  Speakers spk_output; // output format

  bool     stream;     // filter has started a stream
  bool     flushing;   // filter must flush the started stream

  void update_formats()
  {
    spk_input = filter->get_input();
    spk_output = filter->get_output();

    // output format may depend on data for some input formats
    // and may not depend for others so status may change...
    // see [f3] rule
    ofdd = (spk_output == spk_unknown);
  }

  void check_formats(const char *caller)
  {
    // check input format
    if (filter->get_input() != spk_input)
      log->err("[k2] %s: input format was illegaly changed", caller);
    spk_input = filter->get_input(); // suppress this error report afterwards

    // check output format
    if (ofdd)
    {
      if (stream && filter->get_output() != spk_output)
        log->err("[x] %s: output format was illegaly changed", caller);
    }
    else
    {
      if (filter->get_output() != spk_output)
        log->err("[f2] %s: output format was illegaly changed", caller);
    }
    spk_output = filter->get_output(); // suppress this error report afterwards
/*
    // check unininitialized state
    // commented because filter may have spk_unknown input format and
    // therefore it may not be empty but have spk_unknonwn input format.
    if ((filter->get_input() == spk_unknown) && !filter->is_empty())
      log->err("[f5] %s: filter is not empty in uninitialized state", caller);
*/
    // check output format in full state
    if ((filter->get_output() == spk_unknown) && !filter->is_empty())
      log->err("[s3] %s: filter generates spk_unknown chunk", caller);

  }

  void check_reset(const char *caller)
  {
    if (ofdd && (filter->get_output() != spk_unknown))
      log->err("[f1] %s: output format did not change to spk_unknown", caller);

    if (!filter->is_empty())
      log->err("[f5] %s: filter is not empty", caller);

    // todo: check buffered data
  }

public:
  FilterTester()
  {
    filter = 0;
    log = 0;
  }

  FilterTester(Filter *_filter, Log *_log)
  {
    filter = _filter;
    log = _log;
    update_formats();
  }

  void link(Filter *_filter, Log *_log)
  {
    filter = _filter;
    log = _log;
    update_formats();
  }

  void unlink()
  {
    filter = 0;
    log = 0;
  }

  void reset()
  {
    if (!filter) return;

    stream = false;
    flushing = false;

    check_formats("before reset()");

    filter->reset();

    check_reset("after reset()");
    check_formats("after reset()");
  }

  /////////////////////////////////////////////////////////
  // Sink interface

  bool query_input(Speakers _spk) const 
  {
    if (!filter) return false;
    return filter->query_input(_spk);
  }

  bool set_input(Speakers _spk)
  {
    if (!filter) return false;

    stream = false;
    flushing = false;

    check_formats("before set_input()");

    bool query = filter->query_input(_spk);
    bool result = filter->set_input(_spk);

    if (query != result)
      log->err("[k3] set_input(): query_input() lies");

    if (result)
    {
      // after successful format change filter must 
      // update input and output formats
      if (filter->get_input() != _spk)
        log->err("[k4] set_input(): new format was not set");
      update_formats();
    }
    else
    {
      // if format change failed input and output must 
      // reamin unchanged or require initialization
      if (filter->get_input() == spk_unknown)
        // filter requires reinit so formats was changed
        update_formats();
      else
        // formats stay unchanged
        check_formats("after set_input()");
    }

    // filter must reset in either case
    check_reset("after set_input()"); 
    return result;
  }

  Speakers get_input() const
  {
    if (!filter) return spk_unknown;
    return filter->get_input();
  }

  bool process(const Chunk *_chunk)
  {
    if (!filter) return false;
    bool input_format_change = false;
    bool query = true;

    // check input parameters
    if (!_chunk)
    {
      log->err("process(): null chunk pointer!!!");
      return false;
    }

    // process() must be called only in empty state
    if (!is_empty())
    {
      log->err("process() is called in full state");
      return false;
    }

    // detect input format change
    if (_chunk->spk != get_input())
    {
      input_format_change = true;
      query = filter->query_input(_chunk->spk);
    }

    check_formats("before process()");

    bool result = filter->process(_chunk);

    if (input_format_change)
    {
      stream = false;
      flushing = false;

      if (query != result)
        log->err("[k3] process(): query_input() lies");

      if (result)
      {
        // successful format change
        // filter must update input and output formats
        if (filter->get_input() != _chunk->spk)
          log->err("[k4] process(): new format was not set");
        update_formats();
      }
      else
      {
        // if format change failed input and output must 
        // reamin unchanged or require initialization
        if (filter->get_input() == spk_unknown)
          // filter requires reinit so formats was changed
          update_formats();
        else
          // formats must stay unchanged
          check_formats("after set_input()");
      }
      // we cannot know ofdd status on process() format change
      // so we have to set it to true (do not check output format)
      ofdd = true;
    }
    else
    {
      if (!result && filter->get_input() == spk_unknown)
        // filter requires reinit so formats was changed
        update_formats();
      else 
        // formats must stay unchanged
        check_formats("after process()");
    }

    // filter starts a stream when it fills
    if (!is_empty())
      stream = true;

    // filter must flush a started stream
    if (_chunk->eos && stream)
      if (is_empty())
        log->err("process(): filter does not want to end the stream");
      else
        flushing = true;

    return result;
  }

  /////////////////////////////////////////////////////////
  // Source interface

  Speakers get_output() const
  {
    if (!filter) return spk_unknown;
    return filter->get_output();
  }

  bool is_empty() const
  {
    if (!filter) return true;
    return filter->is_empty();
  }

  bool get_chunk(Chunk *_chunk)
  {
    if (!filter) return false;

    // check input parameters
    if (!_chunk)
    {
      log->err("get_chunk(): null chunk pointer!!!");
      return false;
    }

    // get_chunk() must be called only in full state
    if (filter->is_empty())
    {
      log->err("get_chunk() is called in empty state");
      return false;
    }

    check_formats("before get_chunk()");

    Speakers spk = filter->get_output();

    if (!filter->get_chunk(_chunk))
      return false;

    if (_chunk->spk != spk)
      log->err("[s1] get_chunk(): get_output() lies");

    // filter has ended a stream
    if (_chunk->eos)
      stream = false;

    check_formats("after get_chunk()");

    // filter continues a stream or starts a new one
    if (!is_empty())
      stream = true;

    // filter must end the stream in flushing mode
    if (flushing && stream && is_empty())
      log->err("get_chunk(): filter did not end the stream");

    return true;
  }
};


#endif