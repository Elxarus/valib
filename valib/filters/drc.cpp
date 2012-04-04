#include <sstream>
#include <iomanip>
#include <math.h>
#include <string.h>
#include "drc.h"

#define LEVEL_MINUS_50DB 0.0031622776601683793319988935444327
#define LEVEL_MINUS_100DB 0.00001
#define LEVEL_PLUS_100DB 100000.0

DRC::DRC(size_t _nsamples)
{
  block     = 0;

  sample[0] = 0;
  sample[1] = 0;

  nsamples  = 0;

  level     = 0;
  factor    = 0;

  drc       = false;
  drc_power = 0;     // dB; this value has meaning of loudness raise at -50dB level
  drc_level = 1.0;   // factor

  gain      = 1.0;   // factor
  attack    = 50.0;  // dB/s
  release   = 50.0;  // dB/s

  // rebuild window
  set_buffer(_nsamples);
}

void
DRC::set_buffer(size_t _nsamples)
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
DRC::get_buffer() const
{
  return nsamples;
}

bool 
DRC::fill_buffer(Chunk &chunk)
{
  size_t n = MIN(chunk.size, nsamples - sample[block]);
  copy_samples(buf[block], sample[block], chunk.samples, 0, spk.nch(), n);

  sample[block] += n;
  chunk.drop_samples(n);
  sync.put(n);
  return sample[block] >= nsamples;
}

void 
DRC::process()
{
  size_t s;
  int ch, nch = spk.nch();
  sample_t spk_level = spk.level;

  ///////////////////////////////////////
  // DRC, gain

  sample_t old_factor = factor;
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

  factor = gain * drc_level;

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

}

///////////////////////////////////////////////////////////
// Filter interface

void 
DRC::reset()
{
  block     = 0;
  sample[0] = 0;
  sample[1] = 0;
  level     = 1.0;
  factor    = 1.0;
  sync.reset();
}

bool 
DRC::process(Chunk &in, Chunk &out)
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
DRC::flush(Chunk &out)
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
DRC::info() const
{
  std::stringstream s;
  s << std::boolalpha << std::fixed << std::setprecision(1);
  s << "Gain: " << value2db(gain) << nl
    << "DRC: " << drc << nl
    << "DRC power: " << drc_power << nl
    << "Attack: " << attack << "dB/s" << nl
    << "Release: " << release << "dB/s" << nl;
  return s.str();
}

size_t 
DRC::next_block()
{
  return (block + 1) & 1;
}
