#include "filter_chain.h"


FilterChain::FilterChain()
{
  first   = 0;
  last    = 0;
}

FilterChain::~FilterChain()
{
  clear();
}

void 
FilterChain::add_first(Filter *_filter, const char *_desc)
{
  Entry *new_entry = new Entry(_filter, _desc);

  if (!first)
  {
    first = new_entry;
    last  = new_entry;
  }
  else
  {
    first->source = new_entry;
    new_entry->sink = first;
    first = new_entry;
  }
}

void 
FilterChain::add_last(Filter *_filter, const char *_desc)
{
  Entry *new_entry = new Entry(_filter, _desc);

  if (!first)
  {
    first = new_entry;
    last  = new_entry;
  }
  else
  {
    last->sink = new_entry;
    new_entry->source = last;
    last = new_entry;
  }
}

void 
FilterChain::clear()
{
  Entry *entry = first;
  Entry *next;

  while (entry)
  {
    next = entry->sink;
    delete entry;
    entry = next;
  }
  
  first   = 0;
  last    = 0;
  reset();
}

bool 
FilterChain::process_internal(Entry *entry)
{
  Chunk chunk;
  while (entry)
  {
    if (entry->is_empty())
      entry = entry->source;
    else
    {
      if (!entry->sink)
        return true;

      if (!entry->get_chunk(&chunk))
        return false;

      entry = entry->sink;

      if (!entry->process(&chunk))
        return false;
    }
  }
  return true;
}


void 
FilterChain::reset()
{
  NullFilter::reset();

  Entry *entry = first;
  while (entry)
  {
    entry->reset();
    entry = entry->sink;
  }
}

bool 
FilterChain::query_input(Speakers _spk) const
{
  if (!first)
    return NullFilter::query_input(_spk);
  else
    return first->query_input(_spk);
}

bool
FilterChain::set_input(Speakers _spk)
{
  // note: set_input() may fail after successful query_input()!

  if (!first)
    return NullFilter::set_input(_spk);

  Entry *entry = first;

  while (entry)
  {
    if (!entry->set_input(_spk))
      return false;

    _spk = entry->get_output();
    if (_spk.format == FORMAT_UNKNOWN)
      // we cannot fully setup filter chain now
      return true;

    entry = entry->sink;
  }
  return true;
}

bool 
FilterChain::process(const Chunk *_chunk)
{
  if (!first)
    return NullFilter::process(_chunk);

  if (!first->process(_chunk))
    return false;
  else
    return process_internal(first);
}

bool 
FilterChain::is_empty()
{
  if (!first)
    return NullFilter::is_empty();
  else
    return last->is_empty();
}
  
Speakers 
FilterChain::get_output() const
{
  if (!first)
    return NullFilter::get_output();
  else
    return last->get_output();
}


bool 
FilterChain::get_chunk(Chunk *_chunk)
{
  if (!first)
    return NullFilter::get_chunk(_chunk);

  if (!last->get_chunk(_chunk))
    return false;
  else
    return process_internal(last);
}
