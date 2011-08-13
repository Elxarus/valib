// TODO!!!
// * use simple convolution for short filters (up to ~32 taps)
//   (FFT filtering is less effective for such lengths)

#include <string.h>
#include "convolver_mch.h"

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


ConvolverMch::ConvolverMch():
  buf_size(0), n(0), c(0),
  pos(0), pre_samples(0), post_samples(0)
{
  for (int ch_name = 0; ch_name < CH_NAMES; ch_name++)
    ver[ch_name] = gen[ch_name].version();

  for (int ch = 0; ch < CH_NAMES; ch++)
  {
    fir[ch] = 0;
    type[ch] = type_pass;
  }
}

ConvolverMch::~ConvolverMch()
{
  uninit();
}

bool
ConvolverMch::fir_changed() const
{
  for (int ch_name = 0; ch_name < CH_NAMES; ch_name++)
    if (ver[ch_name] != gen[ch_name].version())
      return true;
  return false;
}

///////////////////////////////////////////////////////////////////////////////

void
ConvolverMch::set_fir(int ch_name, const FIRGen *new_gen)
{
  gen[ch_name] = new_gen;
}

const FIRGen *
ConvolverMch::get_fir(int ch_name) const
{
  return gen[ch_name].get();
}

void
ConvolverMch::release_fir(int ch_name)
{
  gen[ch_name].release();
}

void
ConvolverMch::set_all_firs(const FIRGen *new_gen[CH_NAMES])
{
  for (int ch_name = 0; ch_name < CH_NAMES; ch_name++)
    gen[ch_name] = new_gen[ch_name];
}

void
ConvolverMch::get_all_firs(const FIRGen *out_gen[CH_NAMES])
{
  for (int ch_name = 0; ch_name < CH_NAMES; ch_name++)
    out_gen[ch_name] = gen[ch_name].get();
}

void
ConvolverMch::release_all_firs()
{
  for (int ch_name = 0; ch_name < CH_NAMES; ch_name++)
    gen[ch_name].release();
}

///////////////////////////////////////////////////////////////////////////////

void
ConvolverMch::process_trivial(samples_t samples, size_t size)
{
  for (int ch = 0; ch < spk.nch(); ch++)
  {
    if (type[ch] == type_zero)
      zero_samples(samples[ch], size);

    if (type[ch] == type_gain)
      gain_samples(fir[ch]->data[0], samples[ch], size);
  }
}

void
ConvolverMch::process_convolve()
{
  int ch, i;
  int nch = spk.nch();
  sample_t *buf_ch, *filter_ch, *delay_ch;

  for (ch = 0; ch < nch; ch++)
    if (type[ch] == type_conv)
      for (int fft_pos = 0; fft_pos < buf_size; fft_pos += n)
      {
        buf_ch = buf[ch] + fft_pos;
        delay_ch = buf[ch] + buf_size;
        filter_ch = filter[ch];

        copy_samples(fft_buf, buf_ch, n);
        zero_samples(fft_buf, n, n);

        fft.rdft(fft_buf);

        fft_buf[0] = filter_ch[0] * fft_buf[0];
        fft_buf[1] = filter_ch[1] * fft_buf[1]; 

        for (i = 1; i < n; i++)
        {
          sample_t re,im;
          re = filter_ch[i*2  ] * fft_buf[i*2] - filter_ch[i*2+1] * fft_buf[i*2+1];
          im = filter_ch[i*2+1] * fft_buf[i*2] + filter_ch[i*2  ] * fft_buf[i*2+1];
          fft_buf[i*2  ] = re;
          fft_buf[i*2+1] = im;
        }

        fft.inv_rdft(fft_buf);

        for (i = 0; i < n; i++)
          buf_ch[i] = fft_buf[i] + delay_ch[i];

        copy_samples(delay_ch, 0, fft_buf, n, n);
      }
}

bool ConvolverMch::init()
{
  int i, ch, ch_name;
  int nch = spk.nch();

  trivial = true;
  int min_point = 0;
  int max_point = 0;

  // Update versions
  for (ch_name = 0; ch_name < CH_NAMES; ch_name++)
    ver[ch_name] = gen[ch_name].version();

  order_t order;
  spk.get_order(order);
  for (ch = 0; ch < nch; ch++)
  {
    ch_name = order[ch];

    safe_delete(fir[ch]);
    fir[ch] = gen[ch_name].make(spk.sample_rate);

    // fir generation error
    if (!fir[ch])
    {
      type[ch] = type_pass;
      continue;
    }

    // validate fir instance
    if (fir[ch]->length <= 0 || fir[ch]->center < 0)
    {
      type[ch] = type_pass;
      safe_delete(fir[ch]);
      continue;
    }

    switch (fir[ch]->type())
    {
      case firt_identity: type[ch] = type_pass; break;
      case firt_zero:     type[ch] = type_zero; break;
      case firt_gain:     type[ch] = type_gain; break;
      default:
        type[ch] = type_conv;
        trivial = false;
        break;
    }

    if (min_point > - fir[ch]->center)
      min_point = -fir[ch]->center;
    if (max_point < fir[ch]->length - fir[ch]->center)
      max_point = fir[ch]->length - fir[ch]->center;
  }

  if (trivial)
    return true;

  /////////////////////////////////////////////////////////
  // Allocate buffers

  n = clp2(max_point - min_point);
  c = -min_point;

  if (n < min_fft_size / 2)
    n = min_fft_size / 2;

  buf_size = n;
  if (buf_size < min_chunk_size)
    buf_size = clp2(min_chunk_size);

  try
  {
    fft.set_length(n * 2);
    filter.allocate(nch, n * 2);
    buf.allocate(nch, buf_size + n);
    fft_buf.allocate(n * 2);
  }
  catch (...)
  {
    uninit();
    return false;
  }

  /////////////////////////////////////////////////////////
  // Build filters

  filter.zero();
  for (ch = 0; ch < nch; ch++)
    if (type[ch] == type_conv)
    {
      for (i = 0; i < fir[ch]->length; i++)
        filter[ch][i + c - fir[ch]->center] = fir[ch]->data[i] / n;
      fft.rdft(filter[ch]);
    }

  /////////////////////////////////////////////////////////
  // Initial state

  pos = 0;
  pre_samples = c;
  post_samples = n - c;
  buf.zero();
  return true;
}

void
ConvolverMch::uninit()
{
  buf_size = 0;
  n = 0;
  c = 0;
  pos = 0;

  trivial = true;
  for (int ch = 0; ch < spk.nch(); ch++)
  {
    safe_delete(fir[ch]);
    type[ch] = type_pass;
  }

  pre_samples = 0;
  post_samples = 0;
}

void
ConvolverMch::reset()
{
  sync.reset();
  pos = 0;
  pre_samples = c;
  post_samples = n - c;
  buf.zero();
}

bool
ConvolverMch::process(Chunk &in, Chunk &out)
{
  int ch;
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

  if (trivial)
  {
    process_trivial(in.samples, in.size);
    out = in;
    in.clear();
    return !out.is_dummy();
  }

  /////////////////////////////////////////////////////////
  // Convolution

  sync.receive_sync(in);

  // Trivial cases:
  // Copy delayed samples to the start of the buffer
  if (pos == 0)
    for (ch = 0; ch < nch; ch++)
      if (type[ch] != type_conv)
        copy_samples(buf[ch], 0, buf[ch], buf_size, c);

  // Accumulate the buffer
  if (pos < buf_size)
  {
    size_t gone = MIN(in.size, size_t(buf_size - pos));
    for (ch = 0; ch < nch; ch++)
      if (type[ch] == type_conv)
        copy_samples(buf[ch], pos, in.samples[ch], 0, gone);
      else
        // Trivial cases are shifted
        copy_samples(buf[ch], c + pos, in.samples[ch], 0, gone);
    pos += (int)gone;
    in.drop_samples(gone);
    sync.put(gone);

    if (pos < buf_size)
      return false;
  }

  pos = 0;
  process_trivial(buf, buf_size);
  process_convolve();

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
ConvolverMch::flush(Chunk &out)
{
  int ch, nch = spk.nch();

  if (!need_flushing())
    return false;

  if (pos == 0)
    for (ch = 0; ch < nch; ch++)
      if (type[ch] != type_conv)
        copy_samples(buf[ch], 0, buf[ch], buf_size, c);

  for (ch = 0; ch < nch; ch++)
    if (type[ch] == type_conv)
      zero_samples(buf[ch], pos, buf_size - pos);

  process_trivial(buf, buf_size);
  process_convolve();

  out.set_linear(buf, pos + c);
  post_samples = 0;
  pos = 0;

  if (pre_samples)
  {
    out.drop_samples(pre_samples);
    pre_samples = 0;
  }
  sync.send_sync_linear(out, spk.sample_rate);
  return true;
}
