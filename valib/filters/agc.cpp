#include <math.h>
#include <memory.h>
#include "agc.h"

#define LEVEL_MINUS_50DB 0.0031622776601683793319988935444327
#define LEVEL_MINUS_100DB 0.00001
#define LEVEL_PLUS_100DB 100000.0

AGC::AGC(size_t _nsamples)
{
  nsamples  = 0;
  sample    = 0;
  block     = 0;
  empty     = true;

  level     = 0;
  factor    = 0;

  // Options
  auto_gain = true;
  normalize = false;

  // Gain control
  master    = 1.0;   // factor
  gain      = 1.0;   // factor
  release   = 50.0;  // dB/s

  // DRC
  drc       = false;
  drc_power = 0;     // dB; this value has meaning of loudness raise at -50dB level
  drc_level = 1.0;   // factor

  input_levels.reset();
  output_levels.reset();

  // rebuild window
  set_buffer(_nsamples);
}

void
AGC::set_buffer(size_t _nsamples)
{
  // allocate buffers
  nsamples = _nsamples;
  buf[0].allocate(NCHANNELS, nsamples);
  buf[1].allocate(NCHANNELS, nsamples);
  w.allocate(2, nsamples);

  // hann window
  double f = 2.0 * M_PI / (nsamples * 2);
  for (size_t i = 0; i < nsamples; i++)
  {
    w[0][i] = 0.5 * (1 - cos(i*f));
    w[1][i] = 0.5 * (1 - cos((i+nsamples)*f));
  }

  // reset
  reset();
}

size_t
AGC::get_buffer() const
{
  return nsamples;
}

bool 
AGC::fill_buffer()
{
  if (sample == 0)
  {
    buf_sync[block] = sync;
    buf_time[block] = time;
    sync = false;
  }

  size_t n = nsamples - sample;
  if (size < n)
  {
    for (int ch = 0; ch < spk.nch(); ch++)
      memcpy(buf[block][ch] + sample, samples[ch], size * sizeof(sample_t));

    sample += size;
    time += size;
    drop_samples(size);

    return false;
  }
  else
  {
    for (int ch = 0; ch < spk.nch(); ch++)
      memcpy(buf[block][ch] + sample, samples[ch], n * sizeof(sample_t));

    sample = 0;
    time += n;
    drop_samples(n);

    return true;
  }
}

void 
AGC::process()
{
  size_t s;
  int ch;
  int nch = spk.nch();
  sample_t spk_level = spk.level;

  sample_t max;
  sample_t *sptr;

  sample_t levels_loc[NCHANNELS];
  memset(levels_loc, 0, sizeof(levels_loc));

  ///////////////////////////////////////
  // Channel levels

  max = 0;
  for (ch = 0; ch < nch; ch++)
  {
    max = 0;
    sptr = buf[block][ch];
    for (s = 0; s < nsamples/8; s++)
    {
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
    }
    levels_loc[ch] = max / spk_level;
  }

  ///////////////////////////////////////
  // Gain, Limiter, DRC

  sample_t old_factor = factor;
  sample_t old_level = level;
  sample_t release_factor;

  // release factor
 
  if (release < 1.0)
    release = 1.0;

  release_factor = pow(release, double(nsamples) / spk.sample_rate);

  // block level

  level = levels_loc[0];
  for (ch = 1; ch < nch; ch++)
    if (level < levels_loc[ch]) 
      level = levels_loc[ch];

  // Here 'level' is realtive block level.
  // Normally, this level should not be greater than 1.0 (0dB level) 
  // but it is possible that some post-processing made it larger.
  // Our task is to decrease the global gain to make the relative 
  // output level <= 1.0.

  // adjust gain (release)

  if (!auto_gain)
    gain = master;
  else
    if (!normalize)
      if (gain * release_factor > master)
        gain = master;
      else
        gain *= release_factor;

  // DRC

  if (drc)
  {
    sample_t compressed_level;

    if (level > LEVEL_MINUS_50DB)
      compressed_level = pow(level, -drc_power/50.0);
    else
      compressed_level = pow(level * LEVEL_PLUS_100DB, drc_power/50.0);
    sample_t released_level = drc_level * release_factor;

    if (level < LEVEL_MINUS_100DB)
      drc_level = 1.0;
    else if (released_level > compressed_level)
      drc_level = compressed_level;
    else
      drc_level = released_level;
  }
  else
    drc_level = 1.0;

  // factor

  factor = gain * drc_level;

  // adjust gain on overflow

  max = MAX(level, old_level) * factor;
  if (auto_gain && max > 1.0)
    if (gain / max > LEVEL_MINUS_50DB)
    {
      factor /= max;
      gain   /= max;
      max     = 1.0;
    }
    else
    {
      factor *= LEVEL_MINUS_50DB / gain;
      max *= LEVEL_MINUS_50DB / gain;
      gain = LEVEL_MINUS_50DB;
    }

  ///////////////////////////////////////
  // Cache levels

  sample_t levels_std[NCHANNELS];

  const int *order = spk.order();
  memset(levels_std, 0, sizeof(levels_std));
  for (ch = 0; ch < nch; ch++)
    levels_std[order[ch]] = levels_loc[ch];

  input_levels.add_levels(buf_time[block], levels_std);

  for (ch = 0; ch < nch; ch++)
    levels_std[order[ch]] *= factor;

  output_levels.add_levels(buf_time[block], levels_std);

  ///////////////////////////////////////
  // Switch blocks

  block  = next_block();

  ///////////////////////////////////////
  // Windowing
  //
  // * full windowing on gain change
  // * simple gain when gain is applied
  // * no windowing if it is no gain applied

  if (!EQUAL_SAMPLES(old_factor, factor))
  {
    // windowing
    for (ch = 0; ch < nch; ch++)
    {
      sptr = buf[block][ch];
      for (s = 0; s < nsamples; s++, sptr++)
        *sptr = *sptr * old_factor * w[1][s] + 
                *sptr * factor * w[0][s];
    }
  }
  else if (!EQUAL_SAMPLES(factor, 1.0))
  {
    // simple gain
    for (ch = 0; ch < nch; ch++)
    {
      sptr = buf[block][ch];
      for (s = 0; s < nsamples; s++, sptr++)
        *sptr *= factor;
    }
  }

  ///////////////////////////////////////
  // Clipping

  if (max > 1.0)
    for (ch = 0; ch < nch; ch++)
    {
      sptr = buf[block][ch];
      for (s = 0; s < nsamples; s++, sptr++)
        if (*sptr > +spk_level) 
          *sptr = +spk_level;
        else
        if (*sptr < -spk_level) 
          *sptr = -spk_level;
    }
}


///////////////////////////////////////////////////////////
// Filter interface

void 
AGC::reset()
{
  NullFilter::reset();

  sample = 0;
  block  = 0;
  empty  = true;
  buf_sync[0] = false;
  buf_sync[1] = false;
  buf_time[0] = 0;
  buf_time[1] = 0;

  level  = 0; //?
  factor = 0; //?

  input_levels.reset();
  output_levels.reset();
}

bool 
AGC::get_chunk(Chunk *_chunk)
{
  if (fill_buffer())
  {
    process();
    if (empty)
    {
      _chunk->set_spk(spk);
      _chunk->set_sync(false, 0);
      _chunk->set_empty();
      empty = false;
    }
    else
    {
      _chunk->set_spk(spk);
      _chunk->set_sync(buf_sync[block], buf_time[block]);
      _chunk->set_samples(buf[block], nsamples);
    }
  }

  // todo: flushing
/*
  // flushing stage 1 - 
  else if (flushing && !empty) 
  {
    // fill sample buffer
    n = nsamples - sample;
    for (ch = 0; ch < spk.nch(); ch++)
      memset(buf[block][ch] + sample, 0, n * sizeof(sample_t));

    process();
    empty = true;
    send_chunk_buffer(_chunk, buf[block], nsamples, false);
  }
  else if (flushing && empty)
  {
    // fill sample buffer
    n = nsamples;
    for (ch = 0; ch < spk.nch(); ch++)
      memset(buf[block][ch] + sample, 0, n * sizeof(sample_t));

    process();
    send_chunk_buffer(_chunk, buf[block], samples, true);
    samples = 0;
  }
  else
  {

  }
*/
  return true;
}
