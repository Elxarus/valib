// TODO!!!
// * use simple convolution for short filters (up to ~32 taps)
//   (FFT filtering is less effective for such lengths)
// * short output chunks are uneffective; do several filtering cycles
//   for short filter lengths (up to ~1000)

#include <string.h>
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
  gen(gen_), fir(0),
  n(0), c(0),
  pos(0), pre_samples(0), post_samples(0),
  state(state_pass)
{
  ver = gen.version();
}

Convolver::~Convolver()
{
  uninit();
}

void
Convolver::convolve()
{
  int i;
  int ch;
  int nch = in_spk.nch();

  for (ch = 0; ch < nch; ch++)
  {
    memset(buf[ch] + n, 0, n * sizeof(sample_t));

    fft.rdft(buf[ch]);

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

    fft.inv_rdft(buf[ch]);
  }

  for (ch = 0; ch < nch; ch++)
    for (i = 0; i < n; i++)
      buf[ch][i] += delay[ch][i];

  for (ch = 0; ch < nch; ch++)
    memcpy(delay[ch], buf[ch] + n, n * sizeof(sample_t));
}

bool Convolver::init(Speakers in_spk_, Speakers &out_spk_)
{
  int i;
  int nch = in_spk_.nch();
  out_spk_ = in_spk_;

  uninit();
  ver = gen.version();
  fir = gen.make(in_spk_.sample_rate);

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

  fft.set_length(n * 2);
  filter.allocate(n * 2);
  buf.allocate(nch, n * 2);
  delay.allocate(nch, n);

  // handle buffer allocation error
  if (!filter.is_allocated() ||
      !buf.is_allocated() ||
      !delay.is_allocated() ||
      !fft.is_ok())
  {
    uninit();
    return false;
  }

  /////////////////////////////////////////////////////////
  // Build the filter

  for (i = 0; i < fir->length; i++)
    filter[i] = fir->data[i] / n;

  for (i = i; i < 2 * n; i++)
    filter[i] = 0;

  fft.rdft(filter);

  state = state_filter;

  /////////////////////////////////////////////////////////
  // Initial state

  pos = 0;
  pre_samples = c;
  post_samples = fir->length - c;
  memset(delay[0], 0, n * nch * sizeof(sample_t));

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
}

void
Convolver::reset_state()
{
  if (state == state_filter)
  {
    pos = 0;
    pre_samples = c;
    post_samples = n - c;
    delay.zero();
  }
}

bool
Convolver::process_samples(samples_t in, size_t in_size, samples_t &out, size_t &out_size, size_t &gone)
{
  int ch;
  int nch = in_spk.nch();

  /////////////////////////////////////////////////////////
  // Trivial filtering

  if (state != state_filter)
  {
    size_t s;
    sample_t gain;
    switch (state)
    {
      case state_zero:
        for (ch = 0; ch < nch; ch++)
          memset(in[ch], 0, in_size * sizeof(sample_t));
        break;

      case state_gain:
        gain = fir->data[0];
        for (ch = 0; ch < nch; ch++)
          for (s = 0; s < in_size; s++)
            in[ch][s] *= gain;
        break;
    }

    out = in;
    out_size = in_size;
    gone = in_size;
    return true;
  }

  /////////////////////////////////////////////////////////
  // Convolution

  if (pos < n)
  {
    gone = MIN(in_size, size_t(n - pos));
    for (ch = 0; ch < nch; ch++)
      memcpy(buf[ch] + pos, in[ch], sizeof(sample_t) * gone);
    pos += (int)gone;

    if (pos < n)
    {
      out.zero();
      out_size = 0;
      return true;
    }
  }

  pos = 0;
  convolve();

  out = buf;
  out_size = n;
  if (pre_samples)
  {
    out += pre_samples;
    out_size -= pre_samples;
    pre_samples = 0;
  }

  return true;
}

bool
Convolver::flush(samples_t &out, size_t &out_size)
{
  if (!need_flushing())
  {
    out.zero();
    out_size = 0;
    return true;
  }

  for (int ch = 0; ch < in_spk.nch(); ch++)
    memset(buf[ch] + pos, 0, (n - pos) * sizeof(sample_t));

  convolve();
  out = buf;
  out_size = pos + c;
  post_samples = 0;
  pos = 0;

  if (pre_samples)
  {
    out += pre_samples;
    out_size -= pre_samples;
    pre_samples = 0;
  }
  return true;
}

bool
Convolver::need_flushing() const
{
  return state == state_filter && post_samples > 0;
}

bool
Convolver::want_reinit() const
{
  return ver != gen.version();
}
