#include <math.h>
#include <memory.h>
#include "agc.h"


#define LEVEL_MINUS_50DB 0.0031622776601683793319988935444327

AGC::AGC(int _nsamples)
{
  nsamples  = 0;
  sample    = 0;
  block     = 0;

  empty     = true;
  timestamp = false;
  time      = 0;

  level     = 0;
  factor    = 0;

  // Options
  auto_gain = true;
  normalize = false;

  // Gain control
  master    = 1.0;
  gain      = 1.0;
  release   = db2value(50);

  // DRC
  drc       = false;
  drc_power = 1.0;
  drc_level = 1.0;

  input_levels.reset();
  output_levels.reset();

  // rebuild window
  set_buffer(_nsamples);
}

void
AGC::set_buffer(int _nsamples)
{
  // allocate buffers
  nsamples = _nsamples;
  samples[0].allocate(NCHANNELS, nsamples);
  samples[1].allocate(NCHANNELS, nsamples);
  w.allocate(2, nsamples);

  // hann window
  double f = 2.0 * M_PI / (nsamples * 2);
  for (int i = 0; i < nsamples; i++)
  {
    w[0][i] = 0.5 * (1 - cos(i*f));
    w[1][i] = 0.5 * (1 - cos((i+nsamples)*f));
  }

  // reset
  reset();
}

int
AGC::get_buffer()
{
  return nsamples;
}

void 
AGC::process()
{
  int ch, s;
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
    sptr = samples[block][ch];
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
    sample_t compressed_level = pow(level, -log10(drc_power)*20/50);
    sample_t released_level = drc_level * release_factor;

    if (released_level > compressed_level)
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
  // Switch blocks

  block  = next_block();
  sample = 0;

  ///////////////////////////////////////
  // Windowing
  // todo: enable windowing only on gain change

  if (!EQUAL_SAMPLES(old_factor, factor))
  {
    // windowing
    for (ch = 0; ch < nch; ch++)
    {
      sptr = samples[block][ch];
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
      sptr = samples[block][ch];
      for (s = 0; s < nsamples; s++, sptr++)
        *sptr *= factor;
    }
  }

  ///////////////////////////////////////
  // Clipping

  if (max > 1.0)
    for (ch = 0; ch < nch; ch++)
    {
      sptr = samples[block][ch];
      for (s = 0; s < nsamples; s++, sptr++)
        if (*sptr > +spk_level) 
          *sptr = +spk_level;
        else
        if (*sptr < -spk_level) 
          *sptr = -spk_level;
    }

  ///////////////////////////////////////
  // Cache levels

  sample_t levels[NCHANNELS];

  const int *order = spk.order();
  memset(levels, 0, sizeof(levels));
  for (ch = 0; ch < nch; ch++)
    levels[order[ch]] = levels_loc[ch];

  input_levels.add_levels(time, levels);

  for (ch = 0; ch < nch; ch++)
    levels[order[ch]] *= factor;

  output_levels.add_levels(time, levels);
}

///////////////////////////////////////////////////////////
// Filter interface

void 
AGC::reset()
{
  chunk.set_empty();

  block  = 0;
  sample = 0;

  level  = 0; //?
  factor = 0; //?

  input_levels.reset();
  output_levels.reset();

  empty     = true;
  timestamp = false;
  time      = 0;
}

bool 
AGC::get_chunk(Chunk *out)
{
  int n, ch;

  // receive input timestamp
  if (chunk.timestamp)
  {
    timestamp = true;
    time = chunk.time - sample;
    chunk.timestamp = false;
  }

  // fill sample buffer
  n = MIN(nsamples - sample, chunk.size);
  for (ch = 0; ch < spk.nch(); ch++)
    memcpy(samples[block][ch] + sample, chunk.samples[ch], n * sizeof(sample_t));

  sample += n;
  chunk.drop(n);

  // fill chunk
  out->set_empty();
  if (sample == nsamples)
  {
    process();
    if (empty)
      empty = false;
    else
    {
      out->set_spk(spk);
      out->set_samples(samples[block], nsamples);
      out->set_time(timestamp, time - nsamples);
      timestamp = false;
    }
    time += nsamples;
  }

  return true;
}
