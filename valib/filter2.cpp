#include "filter2.h"

Filter *Filter2::as_filter()
{ return thunk; }

Filter2::operator Filter *()
{ return thunk; }

Filter *Filter2::operator ->()
{ return thunk; }

Filter2::operator const Filter *() const
{ return thunk; }

const Filter *Filter2::operator ->() const
{ return thunk; }

Filter2::Filter2()
{
  thunk = new FilterThunk(this);
}

Filter2::~Filter2()
{
  safe_delete(thunk);
}

///////////////////////////////////////////////////////////////////////////////

FilterThunk::FilterThunk(Filter2 *f_):
f(f_), flushing(false)
{}

void
FilterThunk::reset()
{
  f->reset();
  in_chunk.set_empty();
  flushing = false;
}

bool
FilterThunk::is_ofdd() const
{
  return false;
}

bool
FilterThunk::query_input(Speakers new_spk) const
{
  return f->can_open(new_spk);
}

bool
FilterThunk::set_input(Speakers new_spk)
{
  in_chunk.set_empty();
  flushing = false;
  if (!f->open(new_spk))
  {
    spk = spk_unknown;
    f->reset();
    return false;
  }

  spk = new_spk;
  f->reset();
  return true;
}

Speakers
FilterThunk::get_input() const
{
  if (!f->is_open())
    return spk_unknown;
  return spk;
}

bool
FilterThunk::process(const Chunk *chunk)
{
  assert(in_chunk.is_empty());

  // ignore dummy chunks
  if (chunk->is_dummy())
    return true;

  // remember data
  in_chunk.set(chunk->rawdata, chunk->samples, chunk->size,
    chunk->sync, chunk->time);

  // format change
  if (spk != chunk->spk)
  {
    flushing = false;
    if (!f->open(chunk->spk))
    {
      in_chunk.set_empty();
      spk = spk_unknown;
      f->reset();
      return false;
    }
    spk = chunk->spk;
    f->reset();
  }

  // flushing
  if (chunk->eos)
    flushing = true;
  return true;
}

Speakers
FilterThunk::get_output() const
{
  return f->get_output();
}

bool
FilterThunk::is_empty() const
{
  return !flushing && in_chunk.is_empty();
}

bool
FilterThunk::get_chunk(Chunk *out_chunk)
{
  Chunk2 out_chunk2;

  // normal processing
  if (!in_chunk.is_empty())
  {
    if (!f->process(in_chunk, out_chunk2))
      return false;

    out_chunk->set(get_output(),
      out_chunk2.rawdata, out_chunk2.samples, out_chunk2.size,
      out_chunk2.sync, out_chunk2.time);
    out_chunk->set_eos(f->eos());
    return true;
  }

  // flushing
  if (flushing)
  {
    if (f->need_flushing())
    {
      f->flush(out_chunk2);
      out_chunk->set(get_output(),
        out_chunk2.rawdata, out_chunk2.samples, out_chunk2.size,
        out_chunk2.sync, out_chunk2.time);
      out_chunk->set_eos(f->eos());
    }
    else
    {
      out_chunk->set_empty(get_output());
      out_chunk->set_eos(true);
      flushing = false;
    }
    return true;
  }

  // dummy
  out_chunk->set_dummy();
  return true;
}
