#include "cache.h"

CacheFilter::CacheFilter(): stream_time(0), buf_time(0), buf_samples(0), pos(0)
{}

CacheFilter::CacheFilter(vtime_t size): stream_time(0), buf_time(0), buf_samples(0), pos(0)
{ set_size(size); }

vtime_t
CacheFilter::get_time() const
{ return stream_time; }

vtime_t
CacheFilter::get_size() const
{ return buf_time; }

void
CacheFilter::set_size(vtime_t size)
{
  buf_time = size;
  buf_samples = buf_time * get_in_spk().sample_rate;
  buf.allocate(get_in_spk().nch(), buf_samples);
  buf.zero();
  pos = 0;
}

size_t
CacheFilter::get_samples(vtime_t time, samples_t samples, size_t size)
{
  int ch;
  int start_pos = buf_samples - int((stream_time - time) * get_in_spk().sample_rate + 0.5);
  if (size > (size_t)buf_samples)
    size = buf_samples;

  if (start_pos < 0) start_pos = 0;
  if (start_pos + size > (size_t)buf_samples)
    start_pos = buf_samples - size;

  start_pos += pos;
  if (start_pos >= buf_samples)
    start_pos -= buf_samples;

  if (start_pos + size > (size_t)buf_samples)
  {
    size_t size1 = buf_samples - start_pos;
    size_t size2 = size - size1;
    for (ch = 0; ch < get_in_spk().nch(); ch++)
      if (samples[ch])
      {
        memcpy(samples[ch], buf[ch] + start_pos, size1 * sizeof(sample_t));
        memcpy(samples[ch] + size1, buf[ch], size2 * sizeof(sample_t));
      }
  }
  else
  {
    for (ch = 0; ch < get_in_spk().nch(); ch++)
      if (samples[ch])
        memcpy(samples[ch], buf[ch] + start_pos, size * sizeof(sample_t));
  }

  return size;
}

///////////////////////////////////////////////////////////////////////////////
// LinearFilter
///////////////////////////////////////////////////////////////////////////////

bool
CacheFilter::init(Speakers spk, Speakers &out_spk)
{
  stream_time = 0;
  buf_samples = buf_time * spk.sample_rate;
  buf.allocate(spk.nch(), buf_samples);
  buf.zero();
  pos = 0;
  return true;
}

void
CacheFilter::reset_state()
{
  stream_time = 0;
  buf.zero();
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
    size_t size1 = buf_samples - pos;
    size_t size2 = pos + size - buf_samples;
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
  pos += size;
  if (pos >= buf_samples)
    pos = 0;
  return true;
}
