/**************************************************************************//**
  \file mixer.h
  \brief Mixer: Matrix mixer filter.
******************************************************************************/

#ifndef VALIB_MIXER_H
#define VALIB_MIXER_H

#include "../buffer.h"
#include "../filter.h"

/**************************************************************************//**
  \class Mixer
  \brief Matrix mixer filter.

  Mixer applies following matrix conversion:

  O = M * I

  Where:
  - O - output sample vector, dimension is out_nch (output number of channels).
  - I - input sample vector, dimension is in_nch (input number of channels).
  - M - conversion matrix, dimension is in_nch x out_nch.

  Input format for the conversion is set by Filter::open() function. Output
  format is set by set_output(), but Speakers::sample_rate is ignored. Mixer
  also converts levels by applying gain: out_spk.level / in_spk.level.

  Matrix may be found automatically, or set directly by the user. This depends
  on \c auto_matrix parameter.

  Automatic matrix generation depends on the following parameters:
  - \c clev - Voice level
  - \c slev - Surround level
  - \c lfelev - LFE level
  - \c expand_stereo - Expand stereo option (upmixing)
  - \c voice_control - Voice control option (allows control of virtual center)
  - \c normalize_matrix - Matrix normalization

  See http://ac3filter.net/wiki/Mixing for more information about automatic
  matrix calculation.

  Mixer may work inplace when number of output channels is less or equal than
  number of input channels. Otherwise it uses buffer for output data. Size of
  the buffer is set at constructor, or with \c buffer_size parameter. So, it is
  inplace/immediate filter, depending on the mixing mode. is_buffered() allows
  to know current mode. Buffer is allocated only buffered mode.

  Mixer also allows to gain each input/output channel and all channels at once.
  Gains are applied to both automatic and manual matrices. Thus, actual matrix
  is following:

  M' = g * g_lev * (I * G_out) * M * (I * G_in)

  Where:
  - \c g - global gain.
  - \c g_lev - level conversion gain (out_spk.level / in_spk.level).
  - \c M - manual or automatic matrix.
  - \c G_in - input_gains, vector of gains for each input channel.
  - \c G_out - output_gains, vector of gains for each output channel.

  \fn Mixer::Mixer(size_t nsamples = 1024);
    \param nsamples Buffer size in samples.

    Constructor with buffer specification. Note that buffer is not allocated
    at the constructor. It is allocated at open() or set_output() in case when
    it is actually required.

  \fn Speakers Mixer::get_output() const;
    Returns the format set at set_output(). Note, mixer does not alter sample
    rate, so sample rate is set from the format passed to open() call.

  \fn bool Mixer::set_output(Speakers spk);
    \param spk Output format.

    Set output format for the mixer. Mixer converts everything, except
    sample_rate. Sample rate is ignored here and set from input format.

  \fn bool Mixer::is_buffered() const;
    Returns true when mixer works in buffered mode, i.e. number of output
    channels is greater than number of input channels.

  \fn size_t Mixer::get_buffer_size() const;
    Returns the size of the buffer for conversion in buffered mode.

  \fn void Mixer::set_buffer_size(size_t nsamples);
    \param nsamples Buffer size in samples.

    Sets the buffer size for conversion in buffered mode.

  \fn void Mixer::calc_matrix()
    Recalculate automatic matrix forcibly. Works even with auto_matrix off.

  \fn void Mixer::get_matrix(matrix_t &matrix) const;
    \param matrix Resulting matrix.

    Returns current matrix, either automatic (auto_matrix enabled) or set
    with set_matrix() (auto_matrix disabled).

  \fn bool Mixer::get_auto_matrix() const;
    Returns current auto_matrix property ('Auto matrix' option).

  \fn bool Mixer::get_normalize_matrix() const;
    Returns current normalize_matrix property ('Normalize matrix' option).

  \fn bool Mixer::get_voice_control() const;
    Retruns current voice_control property ('Voice control option' option).

  \fn bool Mixer::get_expand_stereo() const;
    Returns current expand_stereo property ('Expand stereo' option).

  \fn sample_t Mixer::get_clev() const;
    Retruns current clev property ('Voice level' option).

  \fn sample_t Mixer::get_slev() const;
    Retruns current slev property ('Surround level' option).

  \fn sample_t Mixer::get_lfelev() const;
    Retruns current lfelev property ('LFE level' option).

  \fn sample_t Mixer::get_gain() const;
    Retruns gain applied by the mixer.

  \fn void Mixer::get_input_gains(sample_t input_gains[CH_NAMES]) const;
    \param input_gains Array to receive gains.

    Returns per-channel gains for each input channel.

  \fn void Mixer::get_output_gains(sample_t output_gains[CH_NAMES]) const;
    \param output_gains Array to receive gains.

    Returns per-channel gains for each output channel.

  \fn void Mixer::set_matrix(const matrix_t &matrix);
    \param matrix Matrix to set.

    Set custom mixing matrix. This function works only when auto_matrix is off,
    otherwise this function does nothing.

  \fn void Mixer::set_auto_matrix(bool auto_matrix);
    Set auto_matrix property ('Auto matrix' option).
    
    When turning auto_matrix on, existing matrix is replaced with automatically
    calculated matrix. See http://ac3filter.net/wiki/Mixing for more info about
    automatic matrix calculation.

    When turning auto_matrix off, matrix does not change. I.e. it does not
    revert the matrix set with set_matrix(), but it becomes possible to change
    the matrix.

  \fn void Mixer::set_normalize_matrix(bool normalize_matrix);
    Set normalize_matrix property ('Normalize matrix' option). Martix is
    updated immediately.

  \fn void Mixer::set_voice_control(bool voice_control);
    Set voice_control property ('Voice control' option). Martix is updated
    immediately.

  \fn void Mixer::set_expand_stereo(bool expand_stereo);
    Set expand_stereo property ('Expand stereo' option). Martix is updated
    immediately.

  \fn void Mixer::set_clev(sample_t clev);
    Set clev property ('Voice level' option). Martix is updated immediately.

  \fn void Mixer::set_slev(sample_t slev);
    Set slev property ('Surround level' option). Martix is updated immediately.

  \fn void Mixer::set_lfelev(sample_t lfelev);
    Set lfelev property ('LFE level' option). Martix is updated immediately.

  \fn void Mixer::set_gain(sample_t gain);
    Set gain.
    
    This gain is always applied, even for matrix manually set by set_matrix().
    It is not shown at the matrix returned by get_matrix().

  \fn void Mixer::set_input_gains(const sample_t input_gains[CH_NAMES]);
    \param input_gains Array of gains for each input channel.

    Set individual gains for each input channel.

    This gains are always applied, even for matrix manually set by
    set_matrix(). They are not shown at the matrix returned by get_matrix().

  \fn void Mixer::set_output_gains(const sample_t output_gains[CH_NAMES]);
    \param output_gains Array of gains for each output channel.

    Set individual gains for each output channel.

    This gains are always applied, even for matrix manually set by
    set_matrix(). They are not shown at the matrix returned by get_matrix().

******************************************************************************/

class Mixer : public SamplesFilter
{
public:
  Mixer(size_t nsamples = 1024);

  /////////////////////////////////////////////////////////
  // SamplesFilter overrides

  virtual bool init();
  virtual bool process(Chunk &in, Chunk &out);

  virtual Speakers get_output() const
  { return out_spk; }

  virtual string info() const;

  /////////////////////////////////////////////////////////
  // Mixer interface

  // output format
  bool set_output(Speakers spk);

  // buffer size
  inline bool   is_buffered() const;
  inline size_t get_buffer_size() const;
  inline void   set_buffer_size(size_t nsamples);

  // matrix calculation
  void calc_matrix();

  // options get/set
  inline void     get_matrix(matrix_t &matrix) const;
  inline bool     get_auto_matrix() const;
  inline bool     get_normalize_matrix() const;
  inline bool     get_voice_control() const;
  inline bool     get_expand_stereo() const;
  inline sample_t get_clev() const;
  inline sample_t get_slev() const;
  inline sample_t get_lfelev() const;
  inline sample_t get_gain() const;
  inline void     get_input_gains(sample_t input_gains[CH_NAMES]) const;
  inline void     get_output_gains(sample_t output_gains[CH_NAMES]) const;

  inline void     set_matrix(const matrix_t &matrix);
  inline void     set_auto_matrix(bool auto_matrix);
  inline void     set_normalize_matrix(bool normalize_matrix);
  inline void     set_voice_control(bool voice_control);
  inline void     set_expand_stereo(bool expand_stereo);
  inline void     set_clev(sample_t clev);
  inline void     set_slev(sample_t slev);
  inline void     set_lfelev(sample_t lfelev);
  inline void     set_gain(sample_t gain);
  inline void     set_input_gains(const sample_t input_gains[CH_NAMES]);
  inline void     set_output_gains(const sample_t output_gains[CH_NAMES]);

protected:
  // Speakers
  Speakers out_spk;                //!< output speakers config

  // Buffer
  SampleBuf buf;                   //!< sample buffer
  size_t nsamples;                 //!< buffer size (in samples)

  // Options                      
  bool     auto_matrix;            //!< update matrix automatically
  bool     normalize_matrix;       //!< normalize matrix
  bool     voice_control;          //!< voice control option
  bool     expand_stereo;          //!< expand stereo option

  // Matrix params                
  sample_t clev;                   //!< center mix level
  sample_t slev;                   //!< surround mix level
  sample_t lfelev;                 //!< lfe mix level

  // Gains
  sample_t gain;                   //!< general gain
  sample_t input_gains[CH_NAMES];  //!< input channel gains
  sample_t output_gains[CH_NAMES]; //!< output channel gains

  // Matrix
  matrix_t matrix;                 //!< mixing matrix
  matrix_t m;                      //!< internal matrix representation

  void prepare_matrix();           //!< find internal matrix represenation

public:
  // mixing functions
  void io_mix11(samples_t input, samples_t output, size_t nsamples);
  void io_mix12(samples_t input, samples_t output, size_t nsamples);
  void io_mix13(samples_t input, samples_t output, size_t nsamples);
  void io_mix14(samples_t input, samples_t output, size_t nsamples);
  void io_mix15(samples_t input, samples_t output, size_t nsamples);
  void io_mix16(samples_t input, samples_t output, size_t nsamples);
  void io_mix17(samples_t input, samples_t output, size_t nsamples);
  void io_mix18(samples_t input, samples_t output, size_t nsamples);
  void io_mix21(samples_t input, samples_t output, size_t nsamples);
  void io_mix22(samples_t input, samples_t output, size_t nsamples);
  void io_mix23(samples_t input, samples_t output, size_t nsamples);
  void io_mix24(samples_t input, samples_t output, size_t nsamples);
  void io_mix25(samples_t input, samples_t output, size_t nsamples);
  void io_mix26(samples_t input, samples_t output, size_t nsamples);
  void io_mix27(samples_t input, samples_t output, size_t nsamples);
  void io_mix28(samples_t input, samples_t output, size_t nsamples);
  void io_mix31(samples_t input, samples_t output, size_t nsamples);
  void io_mix32(samples_t input, samples_t output, size_t nsamples);
  void io_mix33(samples_t input, samples_t output, size_t nsamples);
  void io_mix34(samples_t input, samples_t output, size_t nsamples);
  void io_mix35(samples_t input, samples_t output, size_t nsamples);
  void io_mix36(samples_t input, samples_t output, size_t nsamples);
  void io_mix37(samples_t input, samples_t output, size_t nsamples);
  void io_mix38(samples_t input, samples_t output, size_t nsamples);
  void io_mix41(samples_t input, samples_t output, size_t nsamples);
  void io_mix42(samples_t input, samples_t output, size_t nsamples);
  void io_mix43(samples_t input, samples_t output, size_t nsamples);
  void io_mix44(samples_t input, samples_t output, size_t nsamples);
  void io_mix45(samples_t input, samples_t output, size_t nsamples);
  void io_mix46(samples_t input, samples_t output, size_t nsamples);
  void io_mix47(samples_t input, samples_t output, size_t nsamples);
  void io_mix48(samples_t input, samples_t output, size_t nsamples);
  void io_mix51(samples_t input, samples_t output, size_t nsamples);
  void io_mix52(samples_t input, samples_t output, size_t nsamples);
  void io_mix53(samples_t input, samples_t output, size_t nsamples);
  void io_mix54(samples_t input, samples_t output, size_t nsamples);
  void io_mix55(samples_t input, samples_t output, size_t nsamples);
  void io_mix56(samples_t input, samples_t output, size_t nsamples);
  void io_mix57(samples_t input, samples_t output, size_t nsamples);
  void io_mix58(samples_t input, samples_t output, size_t nsamples);
  void io_mix61(samples_t input, samples_t output, size_t nsamples);
  void io_mix62(samples_t input, samples_t output, size_t nsamples);
  void io_mix63(samples_t input, samples_t output, size_t nsamples);
  void io_mix64(samples_t input, samples_t output, size_t nsamples);
  void io_mix65(samples_t input, samples_t output, size_t nsamples);
  void io_mix66(samples_t input, samples_t output, size_t nsamples);
  void io_mix67(samples_t input, samples_t output, size_t nsamples);
  void io_mix68(samples_t input, samples_t output, size_t nsamples);
  void io_mix71(samples_t input, samples_t output, size_t nsamples);
  void io_mix72(samples_t input, samples_t output, size_t nsamples);
  void io_mix73(samples_t input, samples_t output, size_t nsamples);
  void io_mix74(samples_t input, samples_t output, size_t nsamples);
  void io_mix75(samples_t input, samples_t output, size_t nsamples);
  void io_mix76(samples_t input, samples_t output, size_t nsamples);
  void io_mix77(samples_t input, samples_t output, size_t nsamples);
  void io_mix78(samples_t input, samples_t output, size_t nsamples);
  void io_mix81(samples_t input, samples_t output, size_t nsamples);
  void io_mix82(samples_t input, samples_t output, size_t nsamples);
  void io_mix83(samples_t input, samples_t output, size_t nsamples);
  void io_mix84(samples_t input, samples_t output, size_t nsamples);
  void io_mix85(samples_t input, samples_t output, size_t nsamples);
  void io_mix86(samples_t input, samples_t output, size_t nsamples);
  void io_mix87(samples_t input, samples_t output, size_t nsamples);
  void io_mix88(samples_t input, samples_t output, size_t nsamples);

  void ip_mix11(samples_t samples, size_t nsamples);
  void ip_mix12(samples_t samples, size_t nsamples);
  void ip_mix13(samples_t samples, size_t nsamples);
  void ip_mix14(samples_t samples, size_t nsamples);
  void ip_mix15(samples_t samples, size_t nsamples);
  void ip_mix16(samples_t samples, size_t nsamples);
  void ip_mix17(samples_t samples, size_t nsamples);
  void ip_mix18(samples_t samples, size_t nsamples);
  void ip_mix21(samples_t samples, size_t nsamples);
  void ip_mix22(samples_t samples, size_t nsamples);
  void ip_mix23(samples_t samples, size_t nsamples);
  void ip_mix24(samples_t samples, size_t nsamples);
  void ip_mix25(samples_t samples, size_t nsamples);
  void ip_mix26(samples_t samples, size_t nsamples);
  void ip_mix27(samples_t samples, size_t nsamples);
  void ip_mix28(samples_t samples, size_t nsamples);
  void ip_mix31(samples_t samples, size_t nsamples);
  void ip_mix32(samples_t samples, size_t nsamples);
  void ip_mix33(samples_t samples, size_t nsamples);
  void ip_mix34(samples_t samples, size_t nsamples);
  void ip_mix35(samples_t samples, size_t nsamples);
  void ip_mix36(samples_t samples, size_t nsamples);
  void ip_mix37(samples_t samples, size_t nsamples);
  void ip_mix38(samples_t samples, size_t nsamples);
  void ip_mix41(samples_t samples, size_t nsamples);
  void ip_mix42(samples_t samples, size_t nsamples);
  void ip_mix43(samples_t samples, size_t nsamples);
  void ip_mix44(samples_t samples, size_t nsamples);
  void ip_mix45(samples_t samples, size_t nsamples);
  void ip_mix46(samples_t samples, size_t nsamples);
  void ip_mix47(samples_t samples, size_t nsamples);
  void ip_mix48(samples_t samples, size_t nsamples);
  void ip_mix51(samples_t samples, size_t nsamples);
  void ip_mix52(samples_t samples, size_t nsamples);
  void ip_mix53(samples_t samples, size_t nsamples);
  void ip_mix54(samples_t samples, size_t nsamples);
  void ip_mix55(samples_t samples, size_t nsamples);
  void ip_mix56(samples_t samples, size_t nsamples);
  void ip_mix57(samples_t samples, size_t nsamples);
  void ip_mix58(samples_t samples, size_t nsamples);
  void ip_mix61(samples_t samples, size_t nsamples);
  void ip_mix62(samples_t samples, size_t nsamples);
  void ip_mix63(samples_t samples, size_t nsamples);
  void ip_mix64(samples_t samples, size_t nsamples);
  void ip_mix65(samples_t samples, size_t nsamples);
  void ip_mix66(samples_t samples, size_t nsamples);
  void ip_mix67(samples_t samples, size_t nsamples);
  void ip_mix68(samples_t samples, size_t nsamples);
  void ip_mix71(samples_t samples, size_t nsamples);
  void ip_mix72(samples_t samples, size_t nsamples);
  void ip_mix73(samples_t samples, size_t nsamples);
  void ip_mix74(samples_t samples, size_t nsamples);
  void ip_mix75(samples_t samples, size_t nsamples);
  void ip_mix76(samples_t samples, size_t nsamples);
  void ip_mix77(samples_t samples, size_t nsamples);
  void ip_mix78(samples_t samples, size_t nsamples);
  void ip_mix81(samples_t samples, size_t nsamples);
  void ip_mix82(samples_t samples, size_t nsamples);
  void ip_mix83(samples_t samples, size_t nsamples);
  void ip_mix84(samples_t samples, size_t nsamples);
  void ip_mix85(samples_t samples, size_t nsamples);
  void ip_mix86(samples_t samples, size_t nsamples);
  void ip_mix87(samples_t samples, size_t nsamples);
  void ip_mix88(samples_t samples, size_t nsamples);
};

///////////////////////////////////////////////////////////////////////////////
// Mixer inlines
///////////////////////////////////////////////////////////////////////////////

// Buffer management

inline bool
Mixer::is_buffered() const
{
  return out_spk.nch() > spk.nch();
}

inline size_t
Mixer::get_buffer_size() const
{
  return nsamples;
}

inline void 
Mixer::set_buffer_size(size_t _nsamples)
{
  nsamples = _nsamples;
  if (is_buffered())
    buf.allocate(out_spk.nch(), _nsamples);
}

// Options get/set

inline void
Mixer::get_matrix(matrix_t &_matrix) const
{ _matrix = matrix; }

inline bool 
Mixer::get_auto_matrix() const
{ return auto_matrix; }

inline bool 
Mixer::get_normalize_matrix() const
{ return normalize_matrix; }

inline bool 
Mixer::get_voice_control() const
{ return voice_control; }

inline bool 
Mixer::get_expand_stereo() const
{ return expand_stereo; }

inline sample_t 
Mixer::get_clev() const
{ return clev; }

inline sample_t 
Mixer::get_slev() const
{ return slev; }

inline sample_t 
Mixer::get_lfelev() const
{ return lfelev; }

inline sample_t 
Mixer::get_gain() const
{ return gain; }

inline void 
Mixer::get_input_gains(sample_t _input_gains[CH_NAMES]) const
{ memcpy(_input_gains, input_gains, sizeof(input_gains)); }

inline void 
Mixer::get_output_gains(sample_t _output_gains[CH_NAMES]) const
{ memcpy(_output_gains, output_gains, sizeof(output_gains)); }


inline void 
Mixer::set_matrix(const matrix_t &_matrix)
{
  if (!auto_matrix)
  {
    matrix = _matrix;
    prepare_matrix();
  }
}

inline void 
Mixer::set_auto_matrix(bool _auto_matrix)
{
  auto_matrix = _auto_matrix;
  if (auto_matrix) calc_matrix();
}

inline void 
Mixer::set_normalize_matrix(bool _normalize_matrix)
{
  normalize_matrix = _normalize_matrix;
  if (auto_matrix) calc_matrix();
}

inline void 
Mixer::set_voice_control(bool _voice_control)
{
  voice_control = _voice_control;
  if (auto_matrix) calc_matrix();
}

inline void 
Mixer::set_expand_stereo(bool _expand_stereo)
{
  expand_stereo = _expand_stereo;
  if (auto_matrix) calc_matrix();
}

inline void 
Mixer::set_clev(sample_t _clev)
{
  clev = _clev;
  if (auto_matrix) calc_matrix();
}

inline void 
Mixer::set_slev(sample_t _slev)
{
  slev = _slev;
  if (auto_matrix) calc_matrix();
}

inline void 
Mixer::set_lfelev(sample_t _lfelev)
{
  lfelev = _lfelev;
  if (auto_matrix) calc_matrix();
}
inline void 
Mixer::set_gain(sample_t _gain)
{
  gain = _gain;
  prepare_matrix();
}

inline void 
Mixer::set_input_gains(const sample_t _input_gains[CH_NAMES])
{
  memcpy(input_gains, _input_gains, sizeof(input_gains));
  prepare_matrix();
}

inline void 
Mixer::set_output_gains(const sample_t _output_gains[CH_NAMES])
{
  memcpy(output_gains, _output_gains, sizeof(output_gains));
  prepare_matrix();
}

#endif
