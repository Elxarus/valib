#include <sstream>
#include <iomanip>
#include <math.h>
#include <string.h>
#include "agc.h"

AGC::AGC(size_t _nsamples)
{
  block       = 0;

  sample[0]   = 0;
  sample[1]   = 0;

  nsamples  = 0;

  level     = 0;

  // Options
  auto_gain = true;
  normalize = false;

  // Gain control
  master    = 1.0;   // factor
  gain      = 1.0;   // factor
  attack    = 50.0;  // dB/s
  release   = 50.0;  // dB/s

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
  size_t n = MIN(chunk.size, nsamples - sample[block]);
  copy_samples(buf[block], sample[block], chunk.samples, 0, spk.nch(), n);

  sample[block] += n;
  chunk.drop_samples(n);
  sync.put(n);
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

  sample_t old_gain = gain;
  sample_t old_level = level;
  sample_t release_factor;
  sample_t attack_factor;

  // attack/release factor
  if (attack  < 0) attack  = 0;
  if (release < 0) release = 0;

  attack_factor  = db2value(attack  * nsamples / spk.sample_rate);
  release_factor = db2value(release * nsamples / spk.sample_rate);

  // block level

  level = 0;
  for (ch = 0; ch < nch; ch++)
    level = max_samples(level, buf[block][ch], nsamples);
  level = level / spk_level;

  // Here 'level' is block peak-level. Normally, this level should not be 
  // greater than 1.0 (0dB) but it is possible that some post-processing made
  // it larger. Our task is to decrease the global gain to make the output 
  // level <= 1.0.

  if (!auto_gain)
    gain = master;

  // adjust gain on overflow

  sample_t max = MAX(level, old_level) * gain;
  if (auto_gain)
    if (max > 1.0)
    {
      if (max < attack_factor)
        // corrected with no overflow
        gain /= max;
      else
        // overflow, will be clipped
        gain /= attack_factor;
    }
    else if (!normalize)
    {
      // release
      if (max * release_factor > 1.0) // max < 1
        release_factor = 1.0 / max;

      if (gain * release_factor > master)
        gain = master;
      else
        gain *= release_factor;
    }

  ///////////////////////////////////////
  // Switch blocks

  block = next_block();
  if (!sample[block])
    // empty block (start of processing)
    return;

  ///////////////////////////////////////
  // Windowing
  // * full windowing on gain change
  // * simple gain when gain is applied
  // * no windowing when no gain is applied

  if (!EQUAL_SAMPLES(old_gain, gain))
  {
    // windowing
    for (ch = 0; ch < nch; ch++)
    {
      sample_t *sptr = buf[block][ch];
      for (s = 0; s < nsamples; s++, sptr++)
        *sptr = *sptr * (old_gain * w[1][s] + gain * w[0][s]);
    }
  }
  else if (!EQUAL_SAMPLES(gain, 1.0))
    gain_samples(gain, buf[block], nch, nsamples);

  ///////////////////////////////////////
  // Clipping
  // Note that we must clip even in case
  // of previous block overflow...

  if (level * gain > 1.0 || old_level * old_gain > 1.0)
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

  ///////////////////////////////////////
  // Debug

#ifdef _DEBUG
  for (ch = 0; ch < nch; ch++)
  {
    sample_t test_level = fabs(max_samples(0, buf[block][ch], nsamples));
    assert(test_level / spk.level - 1.0 < SAMPLE_THRESHOLD); // floating point
  }
#endif

}

///////////////////////////////////////////////////////////
// Filter interface

void 
AGC::reset()
{
  block     = 0;
  sample[0] = 0;
  sample[1] = 0;
  level     = 1.0;
  gain      = 1.0;
  sync.reset();
}

bool 
AGC::process(Chunk &in, Chunk &out)
{
  sync.receive_sync(in);
  while (fill_buffer(in))
  {
    process();

    // do not send empty first block
    if (!sample[block] && sample[next_block()])
      continue;

    out.set_linear(buf[block], sample[block]);
    sync.send_sync_linear(out, spk.sample_rate);

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

  out.set_linear(buf[block], sample[block]);
  sync.send_sync_linear(out, spk.sample_rate);

  sample[block] = 0;
  return true;
}

string
AGC::info() const
{
  std::stringstream s;
  s << std::boolalpha << std::fixed << std::setprecision(1);
  s << "Gain: " << value2db(master) << nl
    << "Auto gain: " << auto_gain << nl
    << "Normalize: " << normalize << nl
    << "Attack: " << attack << "dB/s" << nl
    << "Release: " << release << "dB/s" << nl;
  return s.str();
}

size_t 
AGC::next_block()
{
  return (block + 1) & 1;
}
