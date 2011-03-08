// TODO!!!
// * use simple convolution for short filters (up to ~32 taps)
//   (FFT filtering is less effective for such lengths)

#include <string.h>
#include "convolver.h"

static const int min_fft_size = 16;
static const int min_chunk_size = 1024;

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
  buf_size(0), n(0), c(0),
  pos(0), pre_samples(0), post_samples(0),
  state(state_pass)
{
  ver = gen.version();
}

Convolver::~Convolver()
{
  uninit();
}

bool
Convolver::fir_changed() const
{
  return ver != gen.version();
}

void
Convolver::convolve()
{
  int i;
  int ch, nch = spk.nch();
  sample_t *buf_ch, *delay_ch;

  for (ch = 0; ch < nch; ch++)
    for (int fft_pos = 0; fft_pos < buf_size; fft_pos += n)
    {
      buf_ch = buf[ch] + fft_pos;
      delay_ch = buf[ch] + buf_size;

      copy_samples(fft_buf, buf_ch, n);
      zero_samples(fft_buf, n, n);

      fft.rdft(fft_buf);

      fft_buf[0] = filter[0] * fft_buf[0];
      fft_buf[1] = filter[1] * fft_buf[1]; 

      for (i = 1; i < n; i++)
      {
        sample_t re,im;
        re = filter[i*2  ] * fft_buf[i*2] - filter[i*2+1] * fft_buf[i*2+1];
        im = filter[i*2+1] * fft_buf[i*2] + filter[i*2  ] * fft_buf[i*2+1];
        fft_buf[i*2  ] = re;
        fft_buf[i*2+1] = im;
      }

      fft.inv_rdft(fft_buf);

      for (i = 0; i < n; i++)
        buf_ch[i] = fft_buf[i] + delay_ch[i];

      copy_samples(delay_ch, 0, fft_buf, n, n);
    }
}

bool Convolver::init()
{
  int i;
  int nch = spk.nch();

  uninit();
  ver = gen.version();
  fir = gen.make(spk.sample_rate);

  if (!fir)
  {
    state = state_pass;
    return true;
  }

  switch (fir->type())
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

  if (n < min_fft_size / 2)
    n = min_fft_size / 2;

  buf_size = n;
  if (buf_size < min_chunk_size)
    buf_size = clp2(min_chunk_size);

  /////////////////////////////////////////////////////////
  // Allocate buffers

  try
  {
    fft.set_length(n * 2);
    filter.allocate(n * 2);
    buf.allocate(nch, buf_size + n);
    fft_buf.allocate(n * 2);
  }
  catch (...)
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
  buf.zero();

  return true;
}

void
Convolver::uninit()
{
  buf_size = 0;
  n = 0;
  c = 0;
  pos = 0;
  pre_samples = 0;
  post_samples = 0;
  state = state_pass;

  safe_delete(fir);
}

void
Convolver::reset()
{
  sync.reset();
  if (state == state_filter)
  {
    pos = 0;
    pre_samples = c;
    post_samples = fir->length - c;
    buf.zero();
  }
}

bool
Convolver::process(Chunk &in, Chunk &out)
{
  int nch = spk.nch();

  /////////////////////////////////////////////////////////
  // Handle FIR change

  if (fir_changed())
  {
    if (need_flushing())
      return flush(out);

    if (!open(spk))
      THROW(EFirChange());
  }

  /////////////////////////////////////////////////////////
  // Trivial filtering

  if (state != state_filter)
  {
    if (state == state_zero)
      zero_samples(in.samples, nch, in.size);

    if (state == state_gain)
      gain_samples(fir->data[0], in.samples, nch, in.size);

    out = in;
    in.clear();
    return !out.is_dummy();
  }

  /////////////////////////////////////////////////////////
  // Convolution

  sync.receive_sync(in);
  if (pos < buf_size)
  {
    size_t gone = MIN(in.size, size_t(buf_size - pos));
    copy_samples(buf, pos, in.samples, 0, nch, gone);

    pos += (int)gone;
    in.drop_samples(gone);
    sync.put(gone);

    if (pos < buf_size)
      return false;
  }

  pos = 0;
  convolve();

  out.set_linear(buf, buf_size);
  if (pre_samples)
  {
    out.drop_samples(pre_samples);
    pre_samples = 0;
  }
  sync.send_sync_linear(out, spk.sample_rate);
  return true;
}

bool
Convolver::flush(Chunk &out)
{
  if (!need_flushing())
    return false;

  zero_samples(buf, pos, spk.nch(), buf_size - pos);
  convolve();

  post_samples = 0;
  pos = 0;

  out.set_linear(buf, pos + c);
  if (pre_samples)
  {
    out.drop_samples(pre_samples);
    pre_samples = 0;
  }
  sync.send_sync_linear(out, spk.sample_rate);
  return true;
}
