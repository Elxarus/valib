// TODO!!!
// * use simple convolution for short filters (up to ~32 taps)
//   (FFT filtering is less effective for such lengths)
// * short output chunks are uneffective; do several filtering cycles
//   for short filter lengths (up to ~1000)

#include <string.h>
#include "../dsp/fftsg_ld.h"
#include "convolver.h"

#define SAFE_DELETE(x) { if (x) delete(x); x = 0; }
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

Convolver::Convolver(const ImpulseResponse *_ir): 
  NullFilter(FORMAT_MASK_LINEAR),
  ir(_ir),
  n(0), c(0),
  filter(0), fft_ip(0), fft_w(0),
  pos(0), pre_samples(0),
  state(state_pass)
{
  buf[0] = 0;
  delay[0] = 0;
}

Convolver::~Convolver()
{
  uninit();
}

void
Convolver::init()
{
  int i, ch;

  uninit();
  ver = ir.version();

  if (spk.is_unknown()) return; // passthrough

  int sample_rate = spk.sample_rate;
  int nch = spk.nch();

  switch (ir.get_type(sample_rate))
  {
    case ir_custom: break; // do filtering, init below
    case ir_zero: state = state_zero; return; // zero output
    default: return; // passthrough
  }

  /////////////////////////////////////////////////////////
  // Decide filter length

  int new_n = ir.min_length(sample_rate);
  if (new_n <= 0) return; // passthrough
  n = clp2(new_n);

  /////////////////////////////////////////////////////////
  // Allocate buffers

  filter   = new sample_t[n * 2];
  fft_ip   = new int[(int)(2 + sqrt(n * 2))];
  fft_w    = new sample_t[n];
  buf[0]   = new sample_t[n * 2 * nch];
  delay[0] = new sample_t[n * nch];

  if (filter == 0 || buf[0] == 0 || fft_ip == 0 || fft_w == 0)
  {
    uninit();
    return; // passthrough
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

  memset(filter, 0, n * 2 * sizeof(sample_t));
  c = ir.get_filter(sample_rate, n, filter);

  for (i = 0; i < n; i++)
    filter[i] /= n;

  fft_ip[0] = 0;
  rdft(n * 2, 1, filter, fft_ip, fft_w);

  state = state_filter;
  reset();
  return;
}

void
Convolver::uninit()
{
  n = 0;
  c = 0;
  pos = 0;
  pre_samples = 0;
  state = state_pass;

  SAFE_DELETE(filter);
  SAFE_DELETE(fft_ip);
  SAFE_DELETE(fft_w);
  SAFE_DELETE(buf[0]);
  SAFE_DELETE(delay[0]);
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

bool
Convolver::set_input(Speakers _spk)
{
  FILTER_SAFE(NullFilter::set_input(_spk));
  init();
  return true;
}

void
Convolver::reset()
{
  NullFilter::reset();

  if (ver != ir.version())
    init();

  if (state == state_filter)
  {
    // Regenerate the response if nessesary
    pos = 0;
    pre_samples = c;
    memset(delay[0], 0, n * spk.nch() * sizeof(sample_t));
  }
}

bool
Convolver::get_chunk(Chunk *chunk)
{
  int ch;

  /////////////////////////////////////////////////////////
  // Do trivial filtering

  switch (state)
  {
    case state_zero:
      for (ch = 0; ch < spk.nch(); ch++)
        memset(samples[ch], 0, size * sizeof(sample_t));

    case state_pass:
      send_chunk_inplace(chunk, size);
      if (ver != ir.version())
      {
        chunk->set_eos();
        init();
      }
      return true;

    case state_filter:
      break;

    default:
      return NullFilter::get_chunk(chunk);
  };

  /////////////////////////////////////////////////////////
  // Filter

  if (ver != ir.version())
  {
    ///////////////////////////////////////////////////////
    // Need to regenerate the impulse response
    // Flush buffered data and regenerate

    for (ch = 0; ch < spk.nch(); ch++)
      memset(buf[ch] + pos, 0, (n - pos) * sizeof(sample_t));

    process_block();
    chunk->set_linear(spk, out, pos + c);
    chunk->set_eos();

    if (pre_samples)
    {
      chunk->samples += pre_samples;
      chunk->size -= pre_samples;
      pre_samples = 0;
    }

    init();
    return true;
  }
  else if (size)
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
    return true;
  }
  else if (flushing)
  {
    ///////////////////////////////////////////////////////
    // Flushing

    for (ch = 0; ch < spk.nch(); ch++)
      memset(buf[ch] + pos, 0, (n - pos) * sizeof(sample_t));

    process_block();
    chunk->set_linear(spk, out, pos + c);
    chunk->set_eos();

    if (pre_samples)
    {
      chunk->samples += pre_samples;
      chunk->size -= pre_samples;
      pre_samples = 0;
    }

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
