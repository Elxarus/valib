#include <math.h>
#include <string.h>
#include "agc.h"
    
#define LEVEL_MINUS_50DB 0.0031622776601683793319988935444327
#define LEVEL_MINUS_100DB 0.00001
#define LEVEL_PLUS_100DB 100000.0

AGC::AGC(size_t _nsamples)
{
  block       = 0;

  sample[0]   = 0;
  sample[1]   = 0;
  buf_sync[0] = false;
  buf_sync[1] = false;
  buf_time[0] = 0;
  buf_time[1] = 0;

  nsamples  = 0;

  level     = 0;
  factor    = 0;

  // Options
  auto_gain = true;
  normalize = false;

  // Gain control
  master    = 1.0;   // factor
  gain      = 1.0;   // factor
  attack    = 50.0;  // dB/s
  release   = 50.0;  // dB/s

  // DRC
  drc       = false;
  drc_power = 0;     // dB; this value has meaning of loudness raise at -50dB level
  drc_level = 1.0;   // factor

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
AGC::fill_buffer(Chunk &chunk)
{
  if (chunk.sync && sample[block] == 0)
  {
    buf_sync[block] = chunk.sync;
    buf_time[block] = chunk.time;
    chunk.sync = false;
    chunk.time = 0;
  }

  size_t n = MIN(chunk.size, nsamples - sample[block]);
  copy_samples(buf[block], sample[block], chunk.samples, 0, spk.nch(), n);

  sample[block] += n;
  chunk.drop_samples(n);
  return sample[block] >= nsamples;
}

void 
AGC::process()
{
  size_t s;
  int ch, nch = spk.nch();
  sample_t spk_level = spk.level;

  ///////////////////////////////////////
  // Gain, Limiter, DRC

  sample_t old_factor = factor;
  sample_t old_level = level;
  sample_t release_factor;
  sample_t attack_factor;

  // attack/release factor
  if (attack  < 0) attack  = 0;
  if (release < 0) release = 0;

  attack_factor  = pow(10.0, attack  * nsamples / spk.sample_rate / 20);
  release_factor = pow(10.0, release * nsamples / spk.sample_rate / 20);

  // block level

  level = 0;
  for (ch = 0; ch < nch; ch++)
    level = max_samples(level, buf[block][ch], nsamples);
  level = level / spk_level;

  // Here 'level' is block peak-level. Normally, this level should not be 
  // greater than 1.0 (0dB) but it is possible that some post-processing made
  // it larger. Our task is to decrease the global gain to make the output 
  // level <= 1.0.

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

  if (!auto_gain)
    gain = master;

  factor = gain * drc_level;

  // adjust gain on overflow

  sample_t max = MAX(level, old_level) * factor;
  if (auto_gain)
    if (max > 1.0)
    {
      if (max < attack_factor)
      {
        // corrected with no overflow
        factor /= max;
        gain   /= max;
        max     = 1.0;
      }
      else
      {
        // overflow, will be clipped
        factor /= attack_factor;
        gain   /= attack_factor;
        max    /= attack_factor;
      }
    }
    else if (!normalize)
    {
      // release
      if (gain * release_factor > master)
        gain = master;
      else
        gain *= release_factor;
    }

  ///////////////////////////////////////
  // Switch blocks

  block  = next_block();

  ///////////////////////////////////////
  // Windowing
  // * full windowing on gain change
  // * simple gain when gain is applied
  // * no windowing when no gain is applied

  if (!EQUAL_SAMPLES(old_factor, factor))
  {
    // windowing
    for (ch = 0; ch < nch; ch++)
    {
      sample_t *sptr = buf[block][ch];
      for (s = 0; s < nsamples; s++, sptr++)
        *sptr = *sptr * (old_factor * w[1][s] + factor * w[0][s]);
    }
  }
  else if (!EQUAL_SAMPLES(factor, 1.0))
    gain_samples(factor, buf[block], nch, nsamples);

  ///////////////////////////////////////
  // Clipping
  // Note that we must clip even in case
  // of previous block overflow...

  if (level * factor > 1.0 || old_level * old_factor > 1.0)
    for (ch = 0; ch < nch; ch++)
    {
      sample_t *sptr = buf[block][ch];
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
  block       = 0;

  sample[0]   = 0;
  sample[1]   = 0;
  buf_sync[0] = false;
  buf_sync[1] = false;
  buf_time[0] = 0;
  buf_time[1] = 0;

  level  = 1.0;
  factor = 1.0;
}

bool 
AGC::process(Chunk &in, Chunk &out)
{
  while (fill_buffer(in))
  {
    process();

    // do not send empty first block
    if (!sample[block] && sample[next_block()])
      continue;

    out.set_linear(
      buf[block], sample[block],
      buf_sync[block], buf_time[block]);

    buf_sync[block] = false;
    sample[block] = 0; // drop block just sent
    return true;
  }

  return false;
}

bool 
AGC::flush(Chunk &out)
{
  if (!sample[0] && !sample[1])
    return false;

  zero_samples(buf[block], sample[block], spk.nch(), nsamples - sample[block]);
  process();

  // do not send empty first block
  if (!sample[block])
  {
    zero_samples(buf[block], sample[block], spk.nch(), nsamples - sample[block]);
    process();
  }

  out.set_linear(
    buf[block], sample[block],
    buf_sync[block], buf_time[block]);

  buf_sync[block] = false;
  sample[block] = 0;
  return true;
}
