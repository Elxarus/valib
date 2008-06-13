// TODO!!!
// * use simple convolution for short filters (up to ~32 taps)
//   (FFT filtering is less effective for such lengths)
// * short output chunks are uneffective; do several filtering cycles
//   for short filter lengths (up to ~1000)

#include <string.h>
#include "../dsp/fftsg_ld.h"
#include "convolver.h"

inline unsigned int clp2(unsigned int x)
{
  // smallest power-of-2 >= x
  x = x - 1;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x + 1;
}


Convolver::Convolver(const FIRGen *gen_): 
  NullFilter(FORMAT_MASK_LINEAR),
  gen(gen_), fir(0),
  n(0), c(0),
  filter(0), fft_ip(0), fft_w(0),
  pos(0), pre_samples(0), post_samples(0),
  state(state_pass)
{
  buf[0] = 0;
  delay[0] = 0;
}

Convolver::~Convolver()
{
  uninit();
}

bool
Convolver::init()
{
  int i, ch;
  int sample_rate = spk.sample_rate;
  int nch = spk.nch();

  if (spk.is_unknown())
    // filter is uninitialized
    return true;

  uninit();
  ver = gen.version();
  fir = gen.make(spk.sample_rate);

  if (!fir)
  {
    state = state_pass;
    return true;
  }

  switch (fir->type)
  {
    case firt_identity: state = state_pass; return true; // passthrough
    case firt_zero:     state = state_zero; return true; // zero filter
    case firt_gain:     state = state_gain; return true; // gain filter
  }

  /////////////////////////////////////////////////////////
  // Decide filter length

  if (fir->length <= 0 || fir->center < 0)
    return false;

  n = clp2(fir->length);
  c = fir->center;

  /////////////////////////////////////////////////////////
  // Allocate buffers

  filter   = new sample_t[n * 2];
  fft_ip   = new int[(int)(2 + sqrt(n * 2))];
  fft_w    = new sample_t[n];
  buf[0]   = new sample_t[n * 2 * nch];
  delay[0] = new sample_t[n * nch];

  // handle buffer allocation error
  if (filter == 0 || buf[0] == 0 || fft_ip == 0 || fft_w == 0)
  {
    uninit();
    return false;
  }

  for (ch = 1; ch < nch; ch++)
    buf[ch] = buf[ch-1] + n * 2;

  for (ch = 1; ch < nch; ch++)
    delay[ch] = delay[ch-1] + n;

  out.zero();
  for (ch = 0; ch < nch; ch++)
    out[ch] = buf[ch];

  /////////////////////////////////////////////////////////
  // Build the filter

  for (i = 0; i < fir->length; i++)
    filter[i] = fir->data[i] / n;

  for (i = i; i < 2 * n; i++)
    filter[i] = 0;

  fft_ip[0] = 0;
  rdft(n * 2, 1, filter, fft_ip, fft_w);

  state = state_filter;

  /////////////////////////////////////////////////////////
  // Initial state

  pos = 0;
  pre_samples = c;
  post_samples = n - c;
  memset(delay[0], 0, n * spk.nch() * sizeof(sample_t));

  return true;
}

void
Convolver::uninit()
{
  n = 0;
  c = 0;
  pos = 0;
  pre_samples = 0;
  post_samples = 0;
  state = state_pass;

  safe_delete(fir);
  safe_delete(filter);
  safe_delete(fft_ip);
  safe_delete(fft_w);
  safe_delete(buf[0]);
  safe_delete(delay[0]);
  out.zero();
}

void
Convolver::process_block()
{
  int ch, i;

  for (ch = 0; ch < spk.nch(); ch++)
  {
    memset(buf[ch] + n, 0, n * sizeof(sample_t));

    rdft(n * 2, 1, buf[ch], fft_ip, fft_w);

    buf[ch][0] = filter[0] * buf[ch][0];
    buf[ch][1] = filter[1] * buf[ch][1]; 

    for (i = 1; i < n; i++)
    {
      sample_t re,im;
      re = filter[i*2  ] * buf[ch][i*2] - filter[i*2+1] * buf[ch][i*2+1];
      im = filter[i*2+1] * buf[ch][i*2] + filter[i*2  ] * buf[ch][i*2+1];
      buf[ch][i*2  ] = re;
      buf[ch][i*2+1] = im;
    }

    rdft(n * 2, -1, buf[ch], fft_ip, fft_w);
  }

  for (ch = 0; ch < spk.nch(); ch++)
    for (i = 0; i < n; i++)
      buf[ch][i] += delay[ch][i];

  for (ch = 0; ch < spk.nch(); ch++)
    memcpy(delay[ch], buf[ch] + n, n * sizeof(sample_t));
}

void
Convolver::reset()
{
  NullFilter::reset();

  sync_helper.reset();

  if (ver != gen.version())
    init();

  if (state == state_filter)
  {
    pos = 0;
    pre_samples = c;
    post_samples = n - c;
    memset(delay[0], 0, n * spk.nch() * sizeof(sample_t));
  }
}

bool
Convolver::set_input(Speakers spk)
{
  FILTER_SAFE(NullFilter::set_input(spk));
  return init();
}

bool
Convolver::process(const Chunk *chunk)
{
  FILTER_SAFE(NullFilter::process(chunk));
  sync_helper.receive_sync(chunk, pos);
  return true;
}

bool
Convolver::get_chunk(Chunk *chunk)
{
  int ch;
  size_t s;

  /////////////////////////////////////////////////////////
  // Rebuild the filter if nessesary
  // In case of trivial filtering only init() will be called

  if (ver != gen.version())
  {
    if (state == state_filter && post_samples > 0)
    {
      /////////////////////////////////////////////////////
      // Flush buffered data

      for (ch = 0; ch < spk.nch(); ch++)
        memset(buf[ch] + pos, 0, (n - pos) * sizeof(sample_t));
      post_samples -= (n - pos);

      process_block();
      chunk->set_linear(spk, out, pos + c);

      if (pre_samples)
      {
        chunk->samples += pre_samples;
        chunk->size -= pre_samples;
        pre_samples = 0;
      }

      sync_helper.send_sync(chunk, 1.0 / spk.sample_rate);
      sync_helper.drop(chunk->size);
      sync = false;

      if (post_samples <= 0)
        chunk->set_eos();

      return true;
    }

    init();
  }

  /////////////////////////////////////////////////////////
  // Do trivial filtering inplace

  switch (state)
  {
    case state_zero:
      for (ch = 0; ch < spk.nch(); ch++)
        memset(samples[ch], 0, size * sizeof(sample_t));
      send_chunk_inplace(chunk, size);
      return true;

    case state_pass:
      send_chunk_inplace(chunk, size);
      return true;

    case state_gain:
      double gain = fir->data[0];
      for (ch = 0; ch < spk.nch(); ch++)
        for (s = 0; s < size; s++)
          samples[ch][s] *= gain;
      send_chunk_inplace(chunk, size);
      return true;
  }

  /////////////////////////////////////////////////////////
  // Filter

  if (size)
  {
    ///////////////////////////////////////////////////////
    // Normal processing

    if (pos < n)
    {
      int l = MIN(int(size), n - pos);
      for (ch = 0; ch < spk.nch(); ch++)
        memcpy(buf[ch] + pos, samples[ch], sizeof(sample_t) * l);
      drop_samples(l);
      pos += l;

      if (pos < n)
      {
        chunk->set_dummy();
        return true;
      }
    }
    pos = 0;

    process_block();
    chunk->set_linear(spk, out, n);
    if (pre_samples)
    {
      chunk->samples += pre_samples;
      chunk->size -= pre_samples;
      pre_samples = 0;
    }
    sync_helper.send_sync(chunk, 1.0 / spk.sample_rate);
    sync_helper.drop(chunk->size);
    sync = false;

    return true;
  }
  else if (flushing)
  {
    ///////////////////////////////////////////////////////
    // Flushing

    if (pre_samples > 0)
    {
      for (ch = 0; ch < spk.nch(); ch++)
        memset(buf[ch] + pos, 0, (n - pos) * sizeof(sample_t));
      post_samples -= (n - pos);

      process_block();
      chunk->set_linear(spk, out, pos + c);
      if (pre_samples)
      {
        chunk->samples += pre_samples;
        chunk->size -= pre_samples;
        pre_samples = 0;
      }
      sync_helper.send_sync(chunk, 1.0 / spk.sample_rate);
      sync_helper.drop(chunk->size);
      sync = false;

      return true;
    }

    chunk->set_dummy();
    chunk->set_eos();
    reset();
    return true;
  }
  else
  {
    ///////////////////////////////////////////////////////
    // Not enough data

    chunk->set_dummy();
    return true;
  }

  // never be here
  assert(false);
  return false;
}
