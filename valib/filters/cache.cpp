#include "cache.h"

CacheFilter::CacheFilter(): stream_time(0), buf_size(0), buf_samples(0), cached_samples(0), pos(0)
{}

CacheFilter::CacheFilter(vtime_t size): stream_time(0), buf_size(0), buf_samples(0), cached_samples(0), pos(0)
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
  int ch, nch = get_in_spk().nch();

  if (new_size < 0) new_size = 0;

  int new_buf_samples = (int)(new_size * get_in_spk().sample_rate + 0.5);
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
      for (ch = 0; ch < nch; ch++)
        memmove(buf[ch], buf[ch] + (pos - new_buf_samples), new_buf_samples * sizeof(sample_t));

    if (new_buf_samples > pos)
      for (ch = 0; ch < nch; ch++)
        memmove(buf[ch] + pos, buf[ch] + pos + (buf_samples - new_buf_samples), (new_buf_samples - pos) * sizeof(sample_t));
  }

  buf.reallocate(get_in_spk().nch(), new_buf_samples);
  if (!buf.is_allocated())
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
    for (ch = 0; ch < nch; ch++)
    {
      memmove(buf[ch] + pos, buf[ch] + pos, (buf_samples - pos) * sizeof(sample_t));
      memset(buf[ch] + pos, 0, (new_pos - pos) * sizeof(sample_t));
    }

  buf_size = new_size;
  buf_samples = new_buf_samples;
  cached_samples = new_cached_samples;
  pos = new_pos;
}

size_t
CacheFilter::get_samples(vtime_t time, samples_t samples, size_t size)
{
  int actual_size = cached_samples;
  if (size < (size_t)cached_samples)
    actual_size = (int)size;

  int start_pos = int((stream_time - time) * get_in_spk().sample_rate + 0.5);

  if (start_pos < actual_size) start_pos = actual_size;
  if (start_pos > cached_samples) start_pos = cached_samples;

  start_pos = buf_samples - start_pos + pos;
  if (start_pos >= buf_samples)
    start_pos -= buf_samples;

  if (start_pos + actual_size > buf_samples)
  {
    int size1 = buf_samples - start_pos;
    int size2 = actual_size - size1;
    for (int ch = 0; ch < get_in_spk().nch(); ch++)
      if (samples[ch])
      {
        memcpy(samples[ch], buf[ch] + start_pos, size1 * sizeof(sample_t));
        memcpy(samples[ch] + size1, buf[ch], size2 * sizeof(sample_t));
      }
  }
  else
  {
    for (int ch = 0; ch < get_in_spk().nch(); ch++)
      if (samples[ch])
        memcpy(samples[ch], buf[ch] + start_pos, actual_size * sizeof(sample_t));
  }

  return actual_size;
}

///////////////////////////////////////////////////////////////////////////////
// LinearFilter
///////////////////////////////////////////////////////////////////////////////

bool
CacheFilter::init(Speakers spk, Speakers &out_spk)
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
CacheFilter::reset_state()
{
  stream_time = 0;
  buf.zero();
  cached_samples = 0;
  pos = 0;
}

void
CacheFilter::sync(vtime_t time)
{ stream_time = time; }

bool
CacheFilter::process_inplace(samples_t samples, size_t size)
{
  int ch;

  stream_time += vtime_t(size) / get_in_spk().sample_rate;
  if (size > (size_t)buf_samples)
  {
    size_t start = size - buf_samples;
    for (ch = 0; ch < get_in_spk().nch(); ch++)
      memcpy(buf[ch], samples[ch] + start, buf_samples * sizeof(sample_t));
    pos = 0;
    return true;
  }

  if (pos + size > (size_t)buf_samples)
  {
    int size1 = buf_samples - pos;
    int size2 = pos + (int)size - buf_samples;
    for (ch = 0; ch < get_in_spk().nch(); ch++)
    {
      memcpy(buf[ch] + pos, samples[ch], size1 * sizeof(sample_t));
      memcpy(buf[ch], samples[ch] + size1, size2 * sizeof(sample_t));
    }
    pos = size2;
    return true;
  }

  for (ch = 0; ch < get_in_spk().nch(); ch++)
    memcpy(buf[ch] + pos, samples[ch], size * sizeof(sample_t));

  pos += (int)size;
  if (pos >= buf_samples)
    pos = 0;

  cached_samples += (int)size;
  if (cached_samples > buf_samples)
    cached_samples = buf_samples;

  return true;
}
