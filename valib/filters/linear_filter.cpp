#include "linear_filter.h"

LinearFilter::LinearFilter()
: flushing(false), size(0), out_size(0), buffered_samples(0)
{}

LinearFilter::~LinearFilter()
{}

void
LinearFilter::reset()
{
  flushing = false;
  samples.zero();
  size = 0;
  out_samples.zero();
  out_size = 0;
  buffered_samples = 0;

  if (want_reinit())
  {
    init(in_spk, out_spk);

    // Timing won't work correctly with SRC, because it's
    // hard to track the internal buffer size of the
    // filter in this case.
    assert(in_spk.sample_rate == out_spk.sample_rate);
  }

  reset_state();
  sync_helper.reset();
}

bool
LinearFilter::is_ofdd() const
{
  return false;
}

bool
LinearFilter::query_input(Speakers spk) const
{ 
  if (!spk.is_linear() || spk.sample_rate == 0 || spk.mask == 0)
    return false;
  return query(spk);
}

bool
LinearFilter::set_input(Speakers spk)
{
  if (!query_input(spk))
  {
    reset();
    return false;
  }

  in_spk = spk;
  FILTER_SAFE(init(spk, out_spk));
  assert(in_spk.sample_rate == out_spk.sample_rate);
  reset();
  return true;
}

Speakers
LinearFilter::get_input() const
{
  return in_spk;
}

bool
LinearFilter::process(const Chunk *chunk)
{
  if (chunk->is_dummy())
    return true;

  if (in_spk != chunk->spk)
    FILTER_SAFE(set_input(chunk->spk));

  sync_helper.receive_sync(chunk, buffered_samples);
  flushing = chunk->eos;
  samples  = chunk->samples;
  size     = chunk->size;

  out_size = 0;
  out_samples.zero();
  while (size > 0 && out_size == 0)
  {
    size_t gone = 0;
    FILTER_SAFE(process_samples(samples, size, out_samples, out_size, gone));
    if (gone >= size) gone = size;
    buffered_samples += gone - out_size;
    size -= gone;
    samples += gone;
  }
  return true;
}

Speakers
LinearFilter::get_output() const
{
  return out_spk;
}

bool
LinearFilter::is_empty() const
{
  return !size && !out_size && !flushing && !want_reinit();
}

bool
LinearFilter::get_chunk(Chunk *chunk)
{
  while (size > 0 && out_size == 0)
  {
    size_t gone = 0;
    FILTER_SAFE(process_samples(samples, size, out_samples, out_size, gone));
    if (gone >= size) gone = size;
    buffered_samples += gone - out_size;
    size -= gone;
    samples += gone;
  }

  if (out_size)
  {
    chunk->set_linear(out_spk, out_samples, out_size);
    sync_helper.send_sync(chunk, 1.0 / in_spk.sample_rate);
    sync_helper.drop(out_size);
    out_size = 0;
    return true;
  }

  bool flush_reinit = want_reinit();
  if ((flush_reinit || flushing) && need_flushing())
  {
    while (out_size == 0 && need_flushing())
      FILTER_SAFE(flush(out_samples, out_size));

    chunk->set_linear(out_spk, out_samples, out_size);
    sync_helper.send_sync(chunk, 1.0 / in_spk.sample_rate);
    sync_helper.drop(out_size);
    out_size = 0;
    return true;
  }

  chunk->set_empty(out_spk);
  if (flush_reinit || flushing)
  {
    chunk->set_eos();
    if (flush_reinit)
    {
      init(in_spk, out_spk);
      assert(in_spk.sample_rate == out_spk.sample_rate);
    }
    reset();
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool
LinearFilter::query(Speakers spk) const
{
  return true;
}

bool
LinearFilter::init(Speakers spk, Speakers &out_spk)
{
  out_spk = spk;
  return true;
}

void
LinearFilter::reset_state()
{}

bool
LinearFilter::process_samples(samples_t in, size_t in_size, samples_t &out, size_t &out_size, size_t &gone)
{
  out = in;
  out_size = in_size;
  gone = in_size;
  FILTER_SAFE(process_inplace(in, in_size));
  return true;
}

bool
LinearFilter::process_inplace(samples_t in, size_t in_size)
{
  return true;
}

bool
LinearFilter::flush(samples_t &out, size_t &out_size)
{
  out.zero();
  out_size = 0;
  return true;
}

bool
LinearFilter::need_flushing() const
{
  return false;
}

bool
LinearFilter::want_reinit() const
{
  return false;
}
