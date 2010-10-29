#include "cache.h"

CacheFilter::CacheFilter():
stream_time(0), buf_size(0), buf_samples(0), cached_samples(0), pos(0)
{}

CacheFilter::CacheFilter(vtime_t size):
stream_time(0), buf_size(0), buf_samples(0), cached_samples(0), pos(0)
{ set_size(size); }

vtime_t
CacheFilter::get_time() const
{ return stream_time; }

vtime_t
CacheFilter::get_size() const
{ return buf_size; }

void
CacheFilter::set_size(vtime_t new_size)
{
  const int nch = spk.nch();

  if (new_size < 0) new_size = 0;

  int new_buf_samples = (int)(new_size * spk.sample_rate + 0.5);
  if (new_buf_samples == buf_samples)
    return;

  int new_cached_samples = cached_samples;
  if (new_buf_samples < buf_samples)
    new_cached_samples = new_buf_samples;

  int new_pos = pos;
  if (new_buf_samples < new_pos)
    new_pos = new_buf_samples;

  // Compact before reallocation
  if (new_buf_samples < buf_samples)
  {
    if (new_buf_samples < pos)
      move_samples(buf, 0, buf, pos - new_buf_samples, nch, new_buf_samples);

    if (new_buf_samples > pos)
      move_samples(buf, pos, buf, pos + buf_samples - new_buf_samples, nch, new_buf_samples - pos);
  }

  try
  {
    buf.reallocate(nch, new_buf_samples);
  }
  catch (...)
  {
    buf_size = 0;
    buf_samples = 0;
    cached_samples = 0;
    pos = 0;
    return;
  }

  // Move data after reallocation
  // Zero the tail
  if (new_buf_samples > buf_samples)
  {
    move_samples(buf, pos + new_buf_samples - buf_samples, buf, pos, nch, buf_samples - pos);
    zero_samples(buf, pos, new_buf_samples - buf_samples);
  }

  buf_size = new_size;
  buf_samples = new_buf_samples;
  cached_samples = new_cached_samples;
  pos = new_pos;
}

size_t
CacheFilter::get_samples(int ch_name, vtime_t time, sample_t *samples, size_t size)
{
  const int nch = spk.nch();

  if (!samples) return 0;
  if (ch_name != CH_NONE && (CH_MASK(ch_name) & spk.mask) == 0) return 0;

  int actual_size = cached_samples;
  if (size < (size_t)cached_samples)
    actual_size = (int)size;

  int start_pos = int((stream_time - time) * spk.sample_rate + 0.5);

  if (start_pos < actual_size) start_pos = actual_size;
  if (start_pos > cached_samples) start_pos = cached_samples;

  start_pos = buf_samples - start_pos + pos;
  if (start_pos >= buf_samples)
    start_pos -= buf_samples;

  int size1 = actual_size;
  int size2 = 0;
  if (start_pos + actual_size > buf_samples)
  {
    size1 = buf_samples - start_pos;
    size2 = actual_size - size1;
  }

  if (ch_name != CH_NONE)
  {
    // Copy one channel
    order_t order;
    spk.get_order(order);

    for (int ch = 0; ch < nch; ch++)
      if (order[ch] == ch_name)
      {
        copy_samples(samples, buf[ch] + start_pos, size1);
        copy_samples(samples + size1, buf[ch], size1);
        break;
      }
  }
  else
  {
    // Sum channels
    copy_samples(samples, buf[0] + start_pos, size1);
    copy_samples(samples + size1, buf[0], size2);
    for (int ch = 1; ch < nch; ch++)
    {
      sum_samples(samples, buf[ch] + start_pos, size1);
      sum_samples(samples + size1, buf[ch], size2);
    }
  }

  return actual_size;
}

///////////////////////////////////////////////////////////////////////////////
// SamplesFilter
///////////////////////////////////////////////////////////////////////////////

bool
CacheFilter::init()
{
  stream_time = 0;
  buf_samples = (int)(buf_size * spk.sample_rate + 0.5);
  buf.allocate(spk.nch(), buf_samples);
  buf.zero();
  cached_samples = 0;
  pos = 0;
  return true;
}

void
CacheFilter::reset()
{
  stream_time = 0;
  buf.zero();
  cached_samples = 0;
  pos = 0;
}

bool
CacheFilter::process(Chunk &in, Chunk &out)
{
  const int nch = spk.nch();
  const samples_t samples = in.samples;
  const size_t size = in.size;

  // Passthrough
  out = in;
  in.clear();
  if (out.is_dummy())
    return false;

  // Receive timestamp
  if (in.sync)
  {
    stream_time = in.time;
    in.sync = false;
    in.time = 0;
  }

  stream_time += vtime_t(size) / spk.sample_rate;
  cached_samples += (int)size;
  if (cached_samples > buf_samples)
    cached_samples = buf_samples;

  if (size > (size_t)buf_samples)
  {
    size_t start = size - buf_samples;
    copy_samples(buf, 0, samples, start, nch, buf_samples);
    pos = 0;
    return true;
  }

  if (pos + size > (size_t)buf_samples)
  {
    int size1 = buf_samples - pos;
    int size2 = pos + (int)size - buf_samples;
    copy_samples(buf, pos, samples, 0, nch, size1);
    copy_samples(buf, 0, samples, size1, nch, size2);
    pos = size2;
    return true;
  }

  copy_samples(buf, pos, samples, 0, nch, size);
  pos += (int)size;
  if (pos >= buf_samples)
    pos = 0;

  return true;
}
