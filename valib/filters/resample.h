#ifndef VALIB_RESAMPLE_H
#define VALIB_RESAMPLE_H

#include "../dsp/fft.h"
#include "../buffer.h"
#include "../filter.h"
#if RESAMPLE_PERF
#include "../win32\cpu.h"
#endif

class Resample : public SamplesFilter
{
protected:
  // user-defined
  double a;   // attenuation factor [dB]
  double q;   // quality (passband width)

  int fs;     // source sample rate [Hz]
  int fd;     // destination sample rate [Hz]
  int nch;    // number fo channels
  double rate;// conversion rate

  // useful in calculations
  int g;      // gcd(fs, fd)
  int l, m;   // l/m - interpolation/decimation factor
  int l1, l2; // stage1/stage2 interpolation factor
  int m1, m2; // stage1/stage2 decimation factor

  // convolution stage filter
  int n1, n1x, n1y; // filter length, x and y lengths
  int c1, c1x, c1y; // center of the filter, x and y coordinates
  sample_t **f1;    // reordered filter [n1y][n1x]
  Samples f1_raw;   // raw filter [n1y * n1x]
  int *order;       // input positions [l]

  // fft stage filter
  int n2, n2b;      // filter size and fft size
  int c2;           // center of the filter
  Samples f2;       // filter [n2b]

  FFT fft;          // fft transformer

  // processing
  int pos_l, pos_m;            // stage1 convolution positions [0..l1), [0..m1)
  int pos1;                    // stage1 buffer position
  SampleBuf buf1;              // stage1 buffer
  SampleBuf buf2;              // stage2 buffer [n2b]
  SampleBuf delay2;            // fft stage delay buffer [n2/m2]
  int shift;                   // fft stage decimation shift
  int pre_samples;             // number of samples to drop from the beginning of output data
  int post_samples;            // number of samples to add to the end of input data

  // stage1_in(): how much input samples required to generate N output samples
  // stage1_out(): how much output samples can be made out of N input samples
  // Note that stage1_out(stage1_in(N)) >= N
  inline int stage1_in(int n)  const { return (n + pos_l) * m1 / l1 - pos_m; }
  inline int stage1_out(int n) const { return ((pos_m + n) * l1 + m1 - 1) / m1 - (pos_m * l1 + m1 - 1) / m1; }

  inline void do_resample();
  inline void do_stage1(sample_t *in[], sample_t *out[], int n_in, int n_out);
  inline void do_stage2();

  bool need_flushing() const
  { return !passthrough() && post_samples > 0 && ((stage1_out(pos1 - c1x) + c2 - shift) / m2 - pre_samples) > 0; }

protected:
  int sample_rate;       // destination sample rate

  Speakers  out_spk;     // output format
  samples_t out_samples; // output buffer
  int       out_size;    // output number of samples

  bool      sync;
  vtime_t   time;

#if RESAMPLE_PERF
public:
  CPUMeter stage1;
  CPUMeter stage2;
#endif

public:
  Resample();
  Resample(int sample_rate, double a = 100, double q = 0.99);
  ~Resample();

  /////////////////////////////////////////////////////////
  // Own interface

  bool set(int sample_rate, double a = 100, double q = 0.99);
  void get(int *sample_rate, double *a = 0, double *q = 0);

  bool set_sample_rate(int _sample_rate) { return set(_sample_rate, a, q); }
  int get_sample_rate() const { return sample_rate; };

  bool set_attenuation(double _a) { return set(sample_rate, _a, q); }
  double get_attenuation() const { return a; }

  bool set_quality(double _q) { return set(sample_rate, a, _q); }
  double get_quality() const { return q; }

  bool passthrough() const
  { return !sample_rate || spk.sample_rate == sample_rate; }

  /////////////////////////////////////////////////////////
  // SamplesFilter overrides

  virtual bool can_open(Speakers spk) const;

  virtual bool init();
  virtual void uninit();

  virtual bool process(Chunk &in, Chunk &out);
  virtual bool flush(Chunk &out);
  virtual void reset();

  virtual Speakers get_output() const
  { return out_spk; }
};

#endif
