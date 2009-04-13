#ifndef PROC_STATE_H
#define PROC_STATE_H

#include "../defs.h"
#include "../fir/eq_fir.h"

struct AudioProcessorState
{
  // Channel order
  int input_order[NCHANNELS];
  int output_order[NCHANNELS];

  // Master gain
  sample_t master;
  sample_t gain;

  // AGC options
  bool auto_gain;
  bool normalize;
  sample_t attack;
  sample_t release;

  // DRC
  bool     drc;
  sample_t drc_power;
  sample_t drc_level;

  // Matrix
  matrix_t matrix;

  // Automatrix options
  bool auto_matrix;
  bool normalize_matrix;
  bool voice_control;
  bool expand_stereo;

  // Automatrix levels
  sample_t clev;
  sample_t slev;
  sample_t lfelev;

  // Input/output gains
  sample_t input_gains[NCHANNELS];
  sample_t output_gains[NCHANNELS];

  // Input/output levels
  sample_t input_levels[NCHANNELS];
  sample_t output_levels[NCHANNELS];

  // SRC
  double src_quality;
  double src_att;

  // Equalizer
  bool     eq;
  size_t   eq_master_nbands;
  EqBand  *eq_master_bands;
  double   eq_master_ripple;
  size_t   eq_nbands[NCHANNELS];
  EqBand  *eq_bands[NCHANNELS];
  double   eq_ripple[NCHANNELS];

  // Spectrum
  unsigned spectrum_length;

  // Bass redirection
  bool     bass_redir;
  int      bass_freq;

  // Delay
  bool     delay;
  int      delay_units;
  float    delays[NCHANNELS];

  AudioProcessorState();
  ~AudioProcessorState();
};

#endif
