#include "filter_chain.h"


FilterChain::Entry::Entry(Filter *_filter, const char *_desc, Entry *_prev)
{
  filter = _filter;
  desc = _desc? strdup(_desc): 0;
  ok = true;
  prev = _prev;
}

FilterChain::Entry::~Entry()
{
  if (desc) delete desc;
}

FilterChain::FilterChain()
{
  last = 0;
}

FilterChain::~FilterChain()
{
  clear();
}

void 
FilterChain::add(Filter *filter, const char *desc)
{
  last = new Entry(filter, desc, last);
}

void 
FilterChain::clear()
{
  Entry *entry = last;
  Entry *prev;

  while (entry)
  {
    prev = entry->prev;
    delete entry;
    entry = prev;
  }
  last = 0;
}

void 
FilterChain::reset()
{
  Entry *entry = last;
  while (entry)
  {
    entry->reset();
    entry = entry->prev;
  }
}

bool 
FilterChain::query_input(Speakers spk) const
{
  Entry *entry = last;
  if (!entry) return true;

  while (entry->prev)
    entry = entry->prev;

  return entry->filter->query_input(spk);
}

bool
FilterChain::set_input(Speakers _spk)
{
  Entry *entry;
  Entry *end = 0;
  while (end != last)
  {
    entry = last;
    while (entry->prev != end)
      entry = entry->prev;

    if (!entry->filter->set_input(_spk))
      return false;

    _spk = entry->filter->get_output();
    end = entry;
  }

  spk = _spk;
  return true;
}

bool 
FilterChain::process(const Chunk *in)
{
  if (!last)
  {
    chunk = *in;
    return true;
  }

  Entry *entry = last;
  while (entry->prev)
    entry = entry->prev;

  return entry->process(in);
}

bool 
FilterChain::is_empty()
{
  if (!last)
    return chunk.is_empty();

  Entry *entry = last;
  while (entry)
  {
    if (!entry->is_empty())
      return false;
    entry = entry->prev;
  }
  return true;
}
  
Speakers 
FilterChain::get_output()
{
  if (!last)
    return spk;

  return last->filter->get_output();
}


bool 
FilterChain::get_chunk(Chunk *out)
{
  if (!last)
  {
    *out = chunk;
    chunk.set_empty();
    return true;
  }

  return last->get_chunk(out);
}
