#include <sstream>
#include <iomanip>
#include <math.h>
#include <string.h>
#include "mixer.h"

static const sample_t LEVEL_SIDE_OF_CENTER_TO_SIDE = sample_t(0.86602540378443864676372317075294);
static const sample_t LEVEL_SIDE_OF_CENTER_TO_FAR_SIDE = 0.5;


typedef void (Mixer::*io_mixfunc_t)(samples_t, samples_t, size_t); // input-output mixing
typedef void (Mixer::*ip_mixfunc_t)(samples_t, size_t);            // in-place mixing

static const io_mixfunc_t io_mix_tbl[NCHANNELS][NCHANNELS] = {
  { &Mixer::io_mix11, &Mixer::io_mix12, &Mixer::io_mix13, &Mixer::io_mix14, &Mixer::io_mix15, &Mixer::io_mix16, &Mixer::io_mix17, &Mixer::io_mix18 },
  { &Mixer::io_mix21, &Mixer::io_mix22, &Mixer::io_mix23, &Mixer::io_mix24, &Mixer::io_mix25, &Mixer::io_mix26, &Mixer::io_mix27, &Mixer::io_mix28 },
  { &Mixer::io_mix31, &Mixer::io_mix32, &Mixer::io_mix33, &Mixer::io_mix34, &Mixer::io_mix35, &Mixer::io_mix36, &Mixer::io_mix37, &Mixer::io_mix38 },
  { &Mixer::io_mix41, &Mixer::io_mix42, &Mixer::io_mix43, &Mixer::io_mix44, &Mixer::io_mix45, &Mixer::io_mix46, &Mixer::io_mix47, &Mixer::io_mix48 },
  { &Mixer::io_mix51, &Mixer::io_mix52, &Mixer::io_mix53, &Mixer::io_mix54, &Mixer::io_mix55, &Mixer::io_mix56, &Mixer::io_mix57, &Mixer::io_mix58 },
  { &Mixer::io_mix61, &Mixer::io_mix62, &Mixer::io_mix63, &Mixer::io_mix64, &Mixer::io_mix65, &Mixer::io_mix66, &Mixer::io_mix67, &Mixer::io_mix68 },
  { &Mixer::io_mix71, &Mixer::io_mix72, &Mixer::io_mix73, &Mixer::io_mix74, &Mixer::io_mix75, &Mixer::io_mix76, &Mixer::io_mix77, &Mixer::io_mix78 },
  { &Mixer::io_mix81, &Mixer::io_mix82, &Mixer::io_mix83, &Mixer::io_mix84, &Mixer::io_mix85, &Mixer::io_mix86, &Mixer::io_mix87, &Mixer::io_mix88 },
};

static const ip_mixfunc_t ip_mix_tbl[NCHANNELS][NCHANNELS] = {
  { &Mixer::ip_mix11, &Mixer::ip_mix12, &Mixer::ip_mix13, &Mixer::ip_mix14, &Mixer::ip_mix15, &Mixer::ip_mix16, &Mixer::ip_mix17, &Mixer::ip_mix18 },
  { &Mixer::ip_mix21, &Mixer::ip_mix22, &Mixer::ip_mix23, &Mixer::ip_mix24, &Mixer::ip_mix25, &Mixer::ip_mix26, &Mixer::ip_mix27, &Mixer::ip_mix28 },
  { &Mixer::ip_mix31, &Mixer::ip_mix32, &Mixer::ip_mix33, &Mixer::ip_mix34, &Mixer::ip_mix35, &Mixer::ip_mix36, &Mixer::ip_mix37, &Mixer::ip_mix38 },
  { &Mixer::ip_mix41, &Mixer::ip_mix42, &Mixer::ip_mix43, &Mixer::ip_mix44, &Mixer::ip_mix45, &Mixer::ip_mix46, &Mixer::ip_mix47, &Mixer::ip_mix48 },
  { &Mixer::ip_mix51, &Mixer::ip_mix52, &Mixer::ip_mix53, &Mixer::ip_mix54, &Mixer::ip_mix55, &Mixer::ip_mix56, &Mixer::ip_mix57, &Mixer::ip_mix58 },
  { &Mixer::ip_mix61, &Mixer::ip_mix62, &Mixer::ip_mix63, &Mixer::ip_mix64, &Mixer::ip_mix65, &Mixer::ip_mix66, &Mixer::ip_mix67, &Mixer::ip_mix68 },
  { &Mixer::ip_mix71, &Mixer::ip_mix72, &Mixer::ip_mix73, &Mixer::ip_mix74, &Mixer::ip_mix75, &Mixer::ip_mix76, &Mixer::ip_mix77, &Mixer::ip_mix78 },
  { &Mixer::ip_mix81, &Mixer::ip_mix82, &Mixer::ip_mix83, &Mixer::ip_mix84, &Mixer::ip_mix85, &Mixer::ip_mix86, &Mixer::ip_mix87, &Mixer::ip_mix88 },
};

Mixer::Mixer(size_t _nsamples)
{
  nsamples = _nsamples;
  out_spk = spk_unknown;

  // Options
  auto_matrix      = true;
  normalize_matrix = true;
  voice_control    = true;
  expand_stereo    = true;

  // Matrix params
  clev   = 1.0;
  slev   = 1.0;
  lfelev = 1.0;

  // Gains
  gain = 1.0;
  for (int ch_name = 0; ch_name < CH_NAMES; ch_name++)
  {
    input_gains[ch_name]  = 1.0;
    output_gains[ch_name] = 1.0;
  }

  // Matrix
  calc_matrix();

  // We don't allocate sample buffer 
  // because we may not need it
}



void 
Mixer::prepare_matrix()
{
  // Convert input matrix into internal form
  // to achieve maximum performance

  // todo: throw out null columns
  // todo: zero null output channels instead of matrixing 
  // todo: do not touch pass-through channels
  // todo: gain channel if possible instead of matrixing

  order_t in_order;
  order_t out_order;
  spk.get_order(in_order);
  out_spk.get_order(out_order);

  sample_t factor = 1.0;

  if (spk.level > 0.0)
    factor = out_spk.level / spk.level * gain;
  else
    factor = out_spk.level * gain;

  for (int ch1 = 0; ch1 < spk.nch(); ch1++)
    for (int ch2 = 0; ch2 < out_spk.nch(); ch2++)
      m[ch1][ch2] = 
        matrix[in_order[ch1]][out_order[ch2]] * 
        input_gains[in_order[ch1]] * 
        output_gains[out_order[ch2]] * 
        factor;
}

bool
Mixer::set_output(Speakers new_spk)
{
  if (!new_spk.is_linear() || new_spk.mask == 0)
    return false;

  out_spk = new_spk;
  out_spk.sample_rate = spk.sample_rate;

  if (is_open())
  {
    if (is_buffered())
      buf.allocate(out_spk.nch(), nsamples);

    if (auto_matrix)
      calc_matrix();

    prepare_matrix();
  }
  return true;
}

///////////////////////////////////////////////////////////
// Filter interface

bool 
Mixer::init()
{
  out_spk.sample_rate = spk.sample_rate;

  if (is_buffered())
    buf.allocate(out_spk.nch(), nsamples);

  if (auto_matrix)
    calc_matrix();

  prepare_matrix();

  return true;
}

bool 
Mixer::process(Chunk &in, Chunk &out)
{
  if (is_buffered())
  {
    // buffered mixing
    size_t n = MIN(nsamples, in.size);
    io_mixfunc_t mixfunc = io_mix_tbl[spk.nch()-1][out_spk.nch()-1];
    (this->*mixfunc)(in.samples, buf, n);

    out.set_linear(buf, n, in.sync, in.time);
    in.drop_samples(n);
    return !out.is_dummy();
  }
  else
  {
    // in-place mixing
    ip_mixfunc_t mixfunc = ip_mix_tbl[spk.nch()-1][out_spk.nch()-1];
    (this->*mixfunc)(in.samples, in.size);

    out = in;
    in.clear();
    return !out.is_dummy();
  }
}

string
Mixer::info() const
{
  int ch_name, in_ch, out_ch;
  std::stringstream s;
  s << std::boolalpha << std::fixed << std::setprecision(1);
  if (is_open())
  {
    s << "Input:" ;
    for (ch_name = 0; ch_name < CH_NAMES; ch_name++)
      if (CH_MASK(ch_name) & spk.mask)
        s << " " << ch_name_short(ch_name);
    s << nl;
  }
  s << "Output:";
  for (ch_name = 0; ch_name < CH_NAMES; ch_name++)
    if (CH_MASK(ch_name) & out_spk.mask)
      s << " " << ch_name_short(ch_name);
  s << nl;

  s << "Buffered: " << (is_open() && is_buffered()) << nl;
  if (is_open() && is_buffered())
    s << "Buffer size: " << nsamples << " samples" << nl;
  s << "Auto matrix: " << auto_matrix << nl
    << "Normalize matrix: " << auto_matrix << nl
    << "Vaoice control: " << voice_control << nl
    << "Expand stereo: " << expand_stereo << nl
    << "Center level: " << value2db(clev) << " dB" << nl
    << "Surround level: " << value2db(slev) << " dB" << nl
    << "LFE level: " << value2db(lfelev) << " dB" << nl;
  s << "Gain: " << value2db(gain) << " dB" << nl;
  s << "Input gains:" << nl;
  for (ch_name = 0; ch_name < CH_NAMES; ch_name++)
    if (!EQUAL_SAMPLES(input_gains[ch_name], 1.0))
      s << ch_name_short(ch_name) << ": " << value2db(input_gains[ch_name]) << " dB" << nl;
  s << "Output gains:" << nl;
  for (ch_name = 0; ch_name < CH_NAMES; ch_name++)
    if (!EQUAL_SAMPLES(output_gains[ch_name], 1.0))
      s << ch_name_short(ch_name) << ": " << value2db(output_gains[ch_name]) << " dB" << nl;
  s << "User matrix:" << nl;
  s << "    ";
  for (in_ch = 0; in_ch < CH_NAMES; in_ch++)
    s << ch_name_short(in_ch) << " ";
  s << nl;
  for (out_ch = 0; out_ch < CH_NAMES; out_ch++)
  {
    s << ch_name_short(out_ch) << ":";
    for (in_ch = 0; in_ch < CH_NAMES; in_ch++)
      s << " " << matrix[out_ch][in_ch];
    s << nl;
  }
  if (is_open())
  {
    s << "Resulting matrix:" << nl;
    order_t input_order;
    order_t output_order;
    spk.get_order(input_order);
    out_spk.get_order(output_order);
    s << "    ";
    for (in_ch = 0; in_ch < spk.nch(); in_ch++)
      s << ch_name_short(input_order[in_ch]) << " ";
    s << nl;
    for (out_ch = 0; out_ch < out_spk.nch(); out_ch++)
    {
      s << ch_name_short(output_order[out_ch]) << ":";
      for (in_ch = 0; in_ch < spk.nch(); in_ch++)
        s << " " << m[out_ch][in_ch];
      s << nl;
    }
  }
  return s.str();
}

///////////////////////////////////////////////////////////////////////////////
// Matrix calculation
///////////////////////////////////////////////////////////////////////////////

void 
Mixer::calc_matrix()
{
  int in_mask    = spk.mask;
  int out_mask   = out_spk.mask;

  int in_nfront  = ((in_mask >> CH_L)  & 1) + ((in_mask >> CH_C)  & 1) + ((in_mask >> CH_R) & 1);
  int in_nrear   = ((in_mask >> CH_SL) & 1) + ((in_mask >> CH_SR) & 1);

  int in_dolby  = NO_RELATION;
  int out_dolby = NO_RELATION;

  if (spk.relation == RELATION_DOLBY ||
      spk.relation == RELATION_DOLBY2)
    in_dolby = spk.relation;

  if (out_spk.relation == RELATION_DOLBY ||
      out_spk.relation == RELATION_DOLBY2)
    out_dolby = out_spk.relation;

  for (int i = 0; i < CH_NAMES; i++)
    for (int j = 0; j < CH_NAMES; j++)
      matrix[i][j] = 0;

  /////////////////////////////////////////////////////////
  // Downmixing

  // Dolby modes are backwards-compatible
  if (in_dolby && out_dolby)
  {
    matrix[CH_L][CH_L] = 1;
    matrix[CH_R][CH_R] = 1;
  }
  // Mix to Dolby Surround/ProLogic/ProLogicII
  else if (out_dolby)
  {
    if (in_nfront >= 2)
    {
      matrix[CH_L][CH_L] = 1;
      matrix[CH_R][CH_R] = 1;
    }
    if (in_nfront != 2)
    {
      matrix[CH_C][CH_L] = LEVEL_3DB * clev;
      matrix[CH_C][CH_R] = LEVEL_3DB * clev;
    }
    if (in_nrear == 1)
    {
      matrix[CH_S][CH_L] = -LEVEL_3DB * slev;
      matrix[CH_S][CH_R] = +LEVEL_3DB * slev;
    }
    else if (in_nrear == 2)
    {
      switch (out_dolby)
      { 
        case RELATION_DOLBY2:
          matrix[CH_SL][CH_L] = -0.8660*slev;
          matrix[CH_SR][CH_L] = -0.5000*slev;
          matrix[CH_SL][CH_R] = +0.5000*slev;
          matrix[CH_SR][CH_R] = +0.8660*slev;
          break;

        case RELATION_DOLBY:
        default:
          matrix[CH_SL][CH_L] = -slev;
          matrix[CH_SR][CH_L] = -slev;
          matrix[CH_SL][CH_R] = +slev;
          matrix[CH_SR][CH_R] = +slev;
          break;
      }
    }
  }
  else
  {
    // Direct routes
    if (in_mask & out_mask & CH_MASK_L)  matrix[CH_L] [CH_L]  = 1.0;
    if (in_mask & out_mask & CH_MASK_R)  matrix[CH_R] [CH_R]  = 1.0;
    if (in_mask & out_mask & CH_MASK_C)  matrix[CH_C] [CH_C]  = clev;
    if (in_mask & out_mask & CH_MASK_SL) matrix[CH_SL][CH_SL] = slev;
    if (in_mask & out_mask & CH_MASK_SR) matrix[CH_SR][CH_SR] = slev;
    if (in_mask & out_mask & CH_MASK_CL) matrix[CH_CL][CH_CL] = 1.0;
    if (in_mask & out_mask & CH_MASK_CR) matrix[CH_CR][CH_CR] = 1.0;
    if (in_mask & out_mask & CH_MASK_BL) matrix[CH_BL][CH_BL] = slev;
    if (in_mask & out_mask & CH_MASK_BC) matrix[CH_BC][CH_BC] = slev;
    if (in_mask & out_mask & CH_MASK_BR) matrix[CH_BR][CH_BR] = slev;
    if (in_mask & out_mask & CH_MASK_LFE) matrix[CH_LFE][CH_LFE] = lfelev;

    // Mix center
    if (in_mask & ~out_mask & CH_MASK_C)
      if ((out_mask & CH_MASK_CL) && (out_mask & CH_MASK_CR))
      {
        matrix[CH_C][CH_CL] = LEVEL_3DB * clev;
        matrix[CH_C][CH_CR] = LEVEL_3DB * clev;
      }
      else if ((out_mask & CH_MASK_L) && (out_mask & CH_MASK_R))
      {
        matrix[CH_C][CH_L] = LEVEL_3DB * clev;
        matrix[CH_C][CH_R] = LEVEL_3DB * clev;
      }

    // Mix left & light
    if (in_mask & ~out_mask & CH_MASK_L)
      if (out_mask & CH_MASK_C)
        matrix[CH_L][CH_C] = 1;

    if (in_mask & ~out_mask & CH_MASK_R)
      if (out_mask & CH_MASK_C)
        matrix[CH_R][CH_C] = 1;

    // Mix left of center & right of center
    if (in_mask & ~out_mask & CH_MASK_CL)
      if ((out_mask & CH_MASK_L) && (out_mask & CH_MASK_C))
      {
        matrix[CH_CL][CH_L] = LEVEL_3DB;
        matrix[CH_CL][CH_C] = LEVEL_3DB;
      }
      else if ((out_mask & CH_MASK_L) && (out_mask & CH_MASK_R))
      {
        matrix[CH_CL][CH_L] = LEVEL_SIDE_OF_CENTER_TO_SIDE;
        matrix[CH_CL][CH_R] = LEVEL_SIDE_OF_CENTER_TO_FAR_SIDE;
      }
      else if (out_mask & CH_MASK_C)
        matrix[CH_CL][CH_C] = 1;

    if (in_mask & ~out_mask & CH_MASK_CR)
      if ((out_mask & CH_MASK_R) && (out_mask & CH_MASK_C))
      {
        matrix[CH_CR][CH_R] = LEVEL_3DB;
        matrix[CH_CR][CH_C] = LEVEL_3DB;
      }
      else if ((out_mask & CH_MASK_L) && (out_mask & CH_MASK_R))
      {
        matrix[CH_CR][CH_R] = LEVEL_SIDE_OF_CENTER_TO_SIDE;
        matrix[CH_CR][CH_L] = LEVEL_SIDE_OF_CENTER_TO_FAR_SIDE;
      }
      else if (out_mask & CH_MASK_C)
        matrix[CH_CR][CH_C] = 1;

    // Mix side left & side right
    if (in_mask & ~out_mask & CH_MASK_SL)
      if (out_mask & CH_MASK_BL)
        matrix[CH_SL][CH_BL] = slev;
      else if (out_mask & CH_MASK_BC)
        matrix[CH_SL][CH_BC] = slev;
      else if (out_mask & CH_MASK_L)
        matrix[CH_SL][CH_L] = slev;
      else if (out_mask & CH_MASK_C)
        matrix[CH_SL][CH_C] = slev;

    if (in_mask & ~out_mask & CH_MASK_SR)
      if (out_mask & CH_MASK_BR)
        matrix[CH_SR][CH_BR] = slev;
      else if (out_mask & CH_MASK_BC)
        matrix[CH_SR][CH_BC] = slev;
      else if (out_mask & CH_MASK_R)
        matrix[CH_SR][CH_R] = slev;
      else if (out_mask & CH_MASK_C)
        matrix[CH_SR][CH_C] = slev;

    // Mix back left & back right
    if (in_mask & ~out_mask & CH_MASK_BL)
      if (out_mask & CH_MASK_SL)
        matrix[CH_BL][CH_SL] = slev;
      else if (out_mask & CH_MASK_BC)
        matrix[CH_BL][CH_BC] = slev;
      else if (out_mask & CH_MASK_L)
        matrix[CH_BL][CH_L] = slev;
      else if (out_mask & CH_MASK_C)
        matrix[CH_BL][CH_C] = slev;

    if (in_mask & ~out_mask & CH_MASK_BR)
      if (out_mask & CH_MASK_SR)
        matrix[CH_BR][CH_SR] = slev;
      else if (out_mask & CH_MASK_BC)
        matrix[CH_BR][CH_BC] = slev;
      else if (out_mask & CH_MASK_R)
        matrix[CH_BR][CH_R] = slev;
      else if (out_mask & CH_MASK_C)
        matrix[CH_BR][CH_C] = slev;

    // Mix back center
    if (in_mask & ~out_mask & CH_MASK_BC)
      if ((out_mask & CH_MASK_BL) && (out_mask & CH_MASK_BR))
      {
        matrix[CH_BC][CH_BL] = LEVEL_3DB * slev;
        matrix[CH_BC][CH_BR] = LEVEL_3DB * slev;
      }
      else if ((out_mask & CH_MASK_SL) && (out_mask & CH_MASK_SR))
      {
        matrix[CH_BC][CH_SL] = LEVEL_3DB * slev;
        matrix[CH_BC][CH_SR] = LEVEL_3DB * slev;
      }
      else if ((out_mask & CH_MASK_L) && (out_mask & CH_MASK_R))
      {
        matrix[CH_BC][CH_L] = LEVEL_3DB * slev;
        matrix[CH_BC][CH_R] = LEVEL_3DB * slev;
      }
      else if (out_mask & CH_MASK_C)
        matrix[CH_BC][CH_C] = slev;

    // Mix LFE
    if (in_mask & ~out_mask & CH_MASK_LFE)
      if ((out_mask & CH_MASK_L) && (out_mask & CH_MASK_R))
      {
        matrix[CH_LFE][CH_L] = LEVEL_3DB * lfelev;
        matrix[CH_LFE][CH_R] = LEVEL_3DB * lfelev;
      }
      else if (out_mask & CH_MASK_C)
        matrix[CH_LFE][CH_C] = lfelev;
  }

  /////////////////////////////////////////////////////////
  // Expand stereo

  if (expand_stereo)
  {
    if ((~in_mask & out_mask & CH_MASK_C) && out_mask != MODE_MONO)
    {
      matrix[CH_L][CH_C] = clev * LEVEL_3DB;
      matrix[CH_R][CH_C] = clev * LEVEL_3DB;
    }

    // No surround at input
    if ((in_mask & (CH_MASK_SL_SR | CH_MASK_BL_BR | CH_MASK_BC)) == 0)
    {
      if ((out_mask & CH_MASK_SL) && (out_mask & CH_MASK_SR))
      {
        matrix[CH_L][CH_SL] = +slev * 0.5;
        matrix[CH_R][CH_SL] = -slev * 0.5;
        matrix[CH_L][CH_SR] = -slev * 0.5;
        matrix[CH_R][CH_SR] = +slev * 0.5;
      }
      if ((out_mask & CH_MASK_BL) && (out_mask & CH_MASK_BR))
      {
        matrix[CH_L][CH_BL] = +slev * 0.5;
        matrix[CH_R][CH_BL] = -slev * 0.5;
        matrix[CH_L][CH_BR] = -slev * 0.5;
        matrix[CH_R][CH_BR] = +slev * 0.5;
      }
      if (out_mask & CH_MASK_BC)
      {
        matrix[CH_L][CH_BC] = +slev * LEVEL_3DB;
        matrix[CH_R][CH_BC] = -slev * LEVEL_3DB;
      }
    }
  }

  /////////////////////////////////////////////////////////
  // Voice control

  if (voice_control)
  {
    sample_t center_level = 0;
    for (int i = 0; i < CH_NAMES; i++)
      center_level += fabs(matrix[i][CH_C]);

    if (
      // Input has left & right but not center
      (in_mask & CH_MASK_L) && (in_mask & CH_MASK_R) && (~in_mask & CH_MASK_C) &&
      // Output has left & right but not center
      (out_mask & CH_MASK_L) && (out_mask & CH_MASK_R) && center_level == 0)
    {
      matrix[CH_L][CH_L] = + 0.5 * (1 + clev);
      matrix[CH_R][CH_L] = - 0.5 * (1 - clev);
      matrix[CH_L][CH_R] = - 0.5 * (1 - clev);
      matrix[CH_R][CH_R] = + 0.5 * (1 + clev);
    }
  }

  /////////////////////////////////////////////////////////
  // Matrix normalization

  if (normalize_matrix)
  {
    double levels[CH_NAMES];
    double max_level;
    double norm;
    int i, j;

    for (i = 0; i < CH_NAMES; i++)
    {
      levels[i] = 0;
      for (j = 0; j < CH_NAMES; j++)
        levels[i] += fabs(matrix[j][i]);
    }

    max_level = levels[0];
    for (i = 1; i < CH_NAMES; i++)
      if (levels[i] > max_level) 
        max_level = levels[i];

    if (max_level > 0)
      norm = 1.0/max_level;
    else
      norm = 1.0;

    for (i = 0; i < CH_NAMES; i++)
      for (j = 0; j < CH_NAMES; j++)
        matrix[j][i] *= norm;
  }

  prepare_matrix();
}



///////////////////////////////////////////////////////////////////////////////
// Mixing functions
///////////////////////////////////////////////////////////////////////////////

void Mixer::io_mix11(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    output[0][s] = buf[0];
  }
}

void Mixer::io_mix12(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[1]  = input[0][s] * m[0][1];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
  }
}

void Mixer::io_mix13(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[2]  = input[0][s] * m[0][2];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
  }
}

void Mixer::io_mix14(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[3]  = input[0][s] * m[0][3];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
  }
}

void Mixer::io_mix15(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[4]  = input[0][s] * m[0][4];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
  }
}

void Mixer::io_mix16(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[5]  = input[0][s] * m[0][5];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
  }
}

void Mixer::io_mix17(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[6]  = input[0][s] * m[0][6];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
  }
}

void Mixer::io_mix18(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[7]  = input[0][s] * m[0][7];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
    output[7][s] = buf[7];
  }
}

void Mixer::io_mix21(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    output[0][s] = buf[0];
  }
}

void Mixer::io_mix22(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
  }
}

void Mixer::io_mix23(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
  }
}

void Mixer::io_mix24(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
  }
}

void Mixer::io_mix25(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
  }
}

void Mixer::io_mix26(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
  }
}

void Mixer::io_mix27(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[6] += input[1][s] * m[1][6];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
  }
}

void Mixer::io_mix28(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[6] += input[1][s] * m[1][6];
    buf[7]  = input[0][s] * m[0][7];
    buf[7] += input[1][s] * m[1][7];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
    output[7][s] = buf[7];
  }
}

void Mixer::io_mix31(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    output[0][s] = buf[0];
  }
}

void Mixer::io_mix32(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
  }
}

void Mixer::io_mix33(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
  }
}

void Mixer::io_mix34(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
  }
}

void Mixer::io_mix35(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
  }
}

void Mixer::io_mix36(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
  }
}

void Mixer::io_mix37(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[6] += input[1][s] * m[1][6];
    buf[6] += input[2][s] * m[2][6];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
  }
}

void Mixer::io_mix38(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[6] += input[1][s] * m[1][6];
    buf[6] += input[2][s] * m[2][6];
    buf[7]  = input[0][s] * m[0][7];
    buf[7] += input[1][s] * m[1][7];
    buf[7] += input[2][s] * m[2][7];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
    output[7][s] = buf[7];
  }
}

void Mixer::io_mix41(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    output[0][s] = buf[0];
  }
}

void Mixer::io_mix42(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
  }
}

void Mixer::io_mix43(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
  }
}

void Mixer::io_mix44(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
  }
}

void Mixer::io_mix45(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
  }
}

void Mixer::io_mix46(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
  }
}

void Mixer::io_mix47(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[6] += input[1][s] * m[1][6];
    buf[6] += input[2][s] * m[2][6];
    buf[6] += input[3][s] * m[3][6];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
  }
}

void Mixer::io_mix48(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[6] += input[1][s] * m[1][6];
    buf[6] += input[2][s] * m[2][6];
    buf[6] += input[3][s] * m[3][6];
    buf[7]  = input[0][s] * m[0][7];
    buf[7] += input[1][s] * m[1][7];
    buf[7] += input[2][s] * m[2][7];
    buf[7] += input[3][s] * m[3][7];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
    output[7][s] = buf[7];
  }
}

void Mixer::io_mix51(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    output[0][s] = buf[0];
  }
}

void Mixer::io_mix52(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
  }
}

void Mixer::io_mix53(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
  }
}

void Mixer::io_mix54(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
  }
}

void Mixer::io_mix55(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
  }
}

void Mixer::io_mix56(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    buf[5] += input[4][s] * m[4][5];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
  }
}

void Mixer::io_mix57(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    buf[5] += input[4][s] * m[4][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[6] += input[1][s] * m[1][6];
    buf[6] += input[2][s] * m[2][6];
    buf[6] += input[3][s] * m[3][6];
    buf[6] += input[4][s] * m[4][6];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
  }
}

void Mixer::io_mix58(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    buf[5] += input[4][s] * m[4][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[6] += input[1][s] * m[1][6];
    buf[6] += input[2][s] * m[2][6];
    buf[6] += input[3][s] * m[3][6];
    buf[6] += input[4][s] * m[4][6];
    buf[7]  = input[0][s] * m[0][7];
    buf[7] += input[1][s] * m[1][7];
    buf[7] += input[2][s] * m[2][7];
    buf[7] += input[3][s] * m[3][7];
    buf[7] += input[4][s] * m[4][7];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
    output[7][s] = buf[7];
  }
}

void Mixer::io_mix61(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    output[0][s] = buf[0];
  }
}

void Mixer::io_mix62(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
  }
}

void Mixer::io_mix63(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
  }
}

void Mixer::io_mix64(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
  }
}

void Mixer::io_mix65(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[4] += input[5][s] * m[5][4];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
  }
}

void Mixer::io_mix66(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[4] += input[5][s] * m[5][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    buf[5] += input[4][s] * m[4][5];
    buf[5] += input[5][s] * m[5][5];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
  }
}

void Mixer::io_mix67(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[4] += input[5][s] * m[5][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    buf[5] += input[4][s] * m[4][5];
    buf[5] += input[5][s] * m[5][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[6] += input[1][s] * m[1][6];
    buf[6] += input[2][s] * m[2][6];
    buf[6] += input[3][s] * m[3][6];
    buf[6] += input[4][s] * m[4][6];
    buf[6] += input[5][s] * m[5][6];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
  }
}

void Mixer::io_mix68(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[4] += input[5][s] * m[5][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    buf[5] += input[4][s] * m[4][5];
    buf[5] += input[5][s] * m[5][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[6] += input[1][s] * m[1][6];
    buf[6] += input[2][s] * m[2][6];
    buf[6] += input[3][s] * m[3][6];
    buf[6] += input[4][s] * m[4][6];
    buf[6] += input[5][s] * m[5][6];
    buf[7]  = input[0][s] * m[0][7];
    buf[7] += input[1][s] * m[1][7];
    buf[7] += input[2][s] * m[2][7];
    buf[7] += input[3][s] * m[3][7];
    buf[7] += input[4][s] * m[4][7];
    buf[7] += input[5][s] * m[5][7];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
    output[7][s] = buf[7];
  }
}

void Mixer::io_mix71(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    output[0][s] = buf[0];
  }
}

void Mixer::io_mix72(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[1] += input[6][s] * m[6][1];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
  }
}

void Mixer::io_mix73(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[1] += input[6][s] * m[6][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[2] += input[6][s] * m[6][2];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
  }
}

void Mixer::io_mix74(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[1] += input[6][s] * m[6][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[2] += input[6][s] * m[6][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    buf[3] += input[6][s] * m[6][3];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
  }
}

void Mixer::io_mix75(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[1] += input[6][s] * m[6][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[2] += input[6][s] * m[6][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    buf[3] += input[6][s] * m[6][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[4] += input[5][s] * m[5][4];
    buf[4] += input[6][s] * m[6][4];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
  }
}

void Mixer::io_mix76(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[1] += input[6][s] * m[6][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[2] += input[6][s] * m[6][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    buf[3] += input[6][s] * m[6][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[4] += input[5][s] * m[5][4];
    buf[4] += input[6][s] * m[6][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    buf[5] += input[4][s] * m[4][5];
    buf[5] += input[5][s] * m[5][5];
    buf[5] += input[6][s] * m[6][5];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
  }
}

void Mixer::io_mix77(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[1] += input[6][s] * m[6][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[2] += input[6][s] * m[6][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    buf[3] += input[6][s] * m[6][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[4] += input[5][s] * m[5][4];
    buf[4] += input[6][s] * m[6][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    buf[5] += input[4][s] * m[4][5];
    buf[5] += input[5][s] * m[5][5];
    buf[5] += input[6][s] * m[6][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[6] += input[1][s] * m[1][6];
    buf[6] += input[2][s] * m[2][6];
    buf[6] += input[3][s] * m[3][6];
    buf[6] += input[4][s] * m[4][6];
    buf[6] += input[5][s] * m[5][6];
    buf[6] += input[6][s] * m[6][6];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
  }
}

void Mixer::io_mix78(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[1] += input[6][s] * m[6][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[2] += input[6][s] * m[6][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    buf[3] += input[6][s] * m[6][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[4] += input[5][s] * m[5][4];
    buf[4] += input[6][s] * m[6][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    buf[5] += input[4][s] * m[4][5];
    buf[5] += input[5][s] * m[5][5];
    buf[5] += input[6][s] * m[6][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[6] += input[1][s] * m[1][6];
    buf[6] += input[2][s] * m[2][6];
    buf[6] += input[3][s] * m[3][6];
    buf[6] += input[4][s] * m[4][6];
    buf[6] += input[5][s] * m[5][6];
    buf[6] += input[6][s] * m[6][6];
    buf[7]  = input[0][s] * m[0][7];
    buf[7] += input[1][s] * m[1][7];
    buf[7] += input[2][s] * m[2][7];
    buf[7] += input[3][s] * m[3][7];
    buf[7] += input[4][s] * m[4][7];
    buf[7] += input[5][s] * m[5][7];
    buf[7] += input[6][s] * m[6][7];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
    output[7][s] = buf[7];
  }
}

void Mixer::io_mix81(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[0] += input[7][s] * m[7][0];
    output[0][s] = buf[0];
  }
}

void Mixer::io_mix82(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[0] += input[7][s] * m[7][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[1] += input[6][s] * m[6][1];
    buf[1] += input[7][s] * m[7][1];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
  }
}

void Mixer::io_mix83(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[0] += input[7][s] * m[7][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[1] += input[6][s] * m[6][1];
    buf[1] += input[7][s] * m[7][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[2] += input[6][s] * m[6][2];
    buf[2] += input[7][s] * m[7][2];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
  }
}

void Mixer::io_mix84(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[0] += input[7][s] * m[7][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[1] += input[6][s] * m[6][1];
    buf[1] += input[7][s] * m[7][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[2] += input[6][s] * m[6][2];
    buf[2] += input[7][s] * m[7][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    buf[3] += input[6][s] * m[6][3];
    buf[3] += input[7][s] * m[7][3];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
  }
}

void Mixer::io_mix85(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[0] += input[7][s] * m[7][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[1] += input[6][s] * m[6][1];
    buf[1] += input[7][s] * m[7][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[2] += input[6][s] * m[6][2];
    buf[2] += input[7][s] * m[7][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    buf[3] += input[6][s] * m[6][3];
    buf[3] += input[7][s] * m[7][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[4] += input[5][s] * m[5][4];
    buf[4] += input[6][s] * m[6][4];
    buf[4] += input[7][s] * m[7][4];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
  }
}

void Mixer::io_mix86(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[0] += input[7][s] * m[7][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[1] += input[6][s] * m[6][1];
    buf[1] += input[7][s] * m[7][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[2] += input[6][s] * m[6][2];
    buf[2] += input[7][s] * m[7][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    buf[3] += input[6][s] * m[6][3];
    buf[3] += input[7][s] * m[7][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[4] += input[5][s] * m[5][4];
    buf[4] += input[6][s] * m[6][4];
    buf[4] += input[7][s] * m[7][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    buf[5] += input[4][s] * m[4][5];
    buf[5] += input[5][s] * m[5][5];
    buf[5] += input[6][s] * m[6][5];
    buf[5] += input[7][s] * m[7][5];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
  }
}

void Mixer::io_mix87(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[0] += input[7][s] * m[7][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[1] += input[6][s] * m[6][1];
    buf[1] += input[7][s] * m[7][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[2] += input[6][s] * m[6][2];
    buf[2] += input[7][s] * m[7][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    buf[3] += input[6][s] * m[6][3];
    buf[3] += input[7][s] * m[7][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[4] += input[5][s] * m[5][4];
    buf[4] += input[6][s] * m[6][4];
    buf[4] += input[7][s] * m[7][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    buf[5] += input[4][s] * m[4][5];
    buf[5] += input[5][s] * m[5][5];
    buf[5] += input[6][s] * m[6][5];
    buf[5] += input[7][s] * m[7][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[6] += input[1][s] * m[1][6];
    buf[6] += input[2][s] * m[2][6];
    buf[6] += input[3][s] * m[3][6];
    buf[6] += input[4][s] * m[4][6];
    buf[6] += input[5][s] * m[5][6];
    buf[6] += input[6][s] * m[6][6];
    buf[6] += input[7][s] * m[7][6];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
  }
}

void Mixer::io_mix88(samples_t input, samples_t output, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = input[0][s] * m[0][0];
    buf[0] += input[1][s] * m[1][0];
    buf[0] += input[2][s] * m[2][0];
    buf[0] += input[3][s] * m[3][0];
    buf[0] += input[4][s] * m[4][0];
    buf[0] += input[5][s] * m[5][0];
    buf[0] += input[6][s] * m[6][0];
    buf[0] += input[7][s] * m[7][0];
    buf[1]  = input[0][s] * m[0][1];
    buf[1] += input[1][s] * m[1][1];
    buf[1] += input[2][s] * m[2][1];
    buf[1] += input[3][s] * m[3][1];
    buf[1] += input[4][s] * m[4][1];
    buf[1] += input[5][s] * m[5][1];
    buf[1] += input[6][s] * m[6][1];
    buf[1] += input[7][s] * m[7][1];
    buf[2]  = input[0][s] * m[0][2];
    buf[2] += input[1][s] * m[1][2];
    buf[2] += input[2][s] * m[2][2];
    buf[2] += input[3][s] * m[3][2];
    buf[2] += input[4][s] * m[4][2];
    buf[2] += input[5][s] * m[5][2];
    buf[2] += input[6][s] * m[6][2];
    buf[2] += input[7][s] * m[7][2];
    buf[3]  = input[0][s] * m[0][3];
    buf[3] += input[1][s] * m[1][3];
    buf[3] += input[2][s] * m[2][3];
    buf[3] += input[3][s] * m[3][3];
    buf[3] += input[4][s] * m[4][3];
    buf[3] += input[5][s] * m[5][3];
    buf[3] += input[6][s] * m[6][3];
    buf[3] += input[7][s] * m[7][3];
    buf[4]  = input[0][s] * m[0][4];
    buf[4] += input[1][s] * m[1][4];
    buf[4] += input[2][s] * m[2][4];
    buf[4] += input[3][s] * m[3][4];
    buf[4] += input[4][s] * m[4][4];
    buf[4] += input[5][s] * m[5][4];
    buf[4] += input[6][s] * m[6][4];
    buf[4] += input[7][s] * m[7][4];
    buf[5]  = input[0][s] * m[0][5];
    buf[5] += input[1][s] * m[1][5];
    buf[5] += input[2][s] * m[2][5];
    buf[5] += input[3][s] * m[3][5];
    buf[5] += input[4][s] * m[4][5];
    buf[5] += input[5][s] * m[5][5];
    buf[5] += input[6][s] * m[6][5];
    buf[5] += input[7][s] * m[7][5];
    buf[6]  = input[0][s] * m[0][6];
    buf[6] += input[1][s] * m[1][6];
    buf[6] += input[2][s] * m[2][6];
    buf[6] += input[3][s] * m[3][6];
    buf[6] += input[4][s] * m[4][6];
    buf[6] += input[5][s] * m[5][6];
    buf[6] += input[6][s] * m[6][6];
    buf[6] += input[7][s] * m[7][6];
    buf[7]  = input[0][s] * m[0][7];
    buf[7] += input[1][s] * m[1][7];
    buf[7] += input[2][s] * m[2][7];
    buf[7] += input[3][s] * m[3][7];
    buf[7] += input[4][s] * m[4][7];
    buf[7] += input[5][s] * m[5][7];
    buf[7] += input[6][s] * m[6][7];
    buf[7] += input[7][s] * m[7][7];
    output[0][s] = buf[0];
    output[1][s] = buf[1];
    output[2][s] = buf[2];
    output[3][s] = buf[3];
    output[4][s] = buf[4];
    output[5][s] = buf[5];
    output[6][s] = buf[6];
    output[7][s] = buf[7];
  }
}

void Mixer::ip_mix11(samples_t samples, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    samples[0][s] = buf[0];
  }
}

void Mixer::ip_mix12(samples_t samples, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[1]  = samples[0][s] * m[0][1];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
  }
}

void Mixer::ip_mix13(samples_t samples, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[2]  = samples[0][s] * m[0][2];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
  }
}

void Mixer::ip_mix14(samples_t samples, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[3]  = samples[0][s] * m[0][3];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
  }
}

void Mixer::ip_mix15(samples_t samples, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[4]  = samples[0][s] * m[0][4];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
  }
}

void Mixer::ip_mix16(samples_t samples, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[5]  = samples[0][s] * m[0][5];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
  }
}

void Mixer::ip_mix17(samples_t samples, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[6]  = samples[0][s] * m[0][6];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
  }
}

void Mixer::ip_mix18(samples_t samples, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[7]  = samples[0][s] * m[0][7];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
    samples[7][s] = buf[7];
  }
}

void Mixer::ip_mix21(samples_t samples, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    samples[0][s] = buf[0];
  }
}

void Mixer::ip_mix22(samples_t samples, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
  }
}

void Mixer::ip_mix23(samples_t samples, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
  }
}

void Mixer::ip_mix24(samples_t samples, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
  }
}

void Mixer::ip_mix25(samples_t samples, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
  }
}

void Mixer::ip_mix26(samples_t samples, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
  }
}

void Mixer::ip_mix27(samples_t samples, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[6] += samples[1][s] * m[1][6];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
  }
}

void Mixer::ip_mix28(samples_t samples, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[6] += samples[1][s] * m[1][6];
    buf[7]  = samples[0][s] * m[0][7];
    buf[7] += samples[1][s] * m[1][7];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
    samples[7][s] = buf[7];
  }
}

void Mixer::ip_mix31(samples_t samples, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    samples[0][s] = buf[0];
  }
}

void Mixer::ip_mix32(samples_t samples, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
  }
}

void Mixer::ip_mix33(samples_t samples, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
  }
}

void Mixer::ip_mix34(samples_t samples, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
  }
}

void Mixer::ip_mix35(samples_t samples, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
  }
}

void Mixer::ip_mix36(samples_t samples, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
  }
}

void Mixer::ip_mix37(samples_t samples, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[6] += samples[1][s] * m[1][6];
    buf[6] += samples[2][s] * m[2][6];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
  }
}

void Mixer::ip_mix38(samples_t samples, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[6] += samples[1][s] * m[1][6];
    buf[6] += samples[2][s] * m[2][6];
    buf[7]  = samples[0][s] * m[0][7];
    buf[7] += samples[1][s] * m[1][7];
    buf[7] += samples[2][s] * m[2][7];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
    samples[7][s] = buf[7];
  }
}

void Mixer::ip_mix41(samples_t samples, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    samples[0][s] = buf[0];
  }
}

void Mixer::ip_mix42(samples_t samples, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
  }
}

void Mixer::ip_mix43(samples_t samples, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
  }
}

void Mixer::ip_mix44(samples_t samples, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
  }
}

void Mixer::ip_mix45(samples_t samples, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
  }
}

void Mixer::ip_mix46(samples_t samples, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
  }
}

void Mixer::ip_mix47(samples_t samples, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[6] += samples[1][s] * m[1][6];
    buf[6] += samples[2][s] * m[2][6];
    buf[6] += samples[3][s] * m[3][6];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
  }
}

void Mixer::ip_mix48(samples_t samples, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[6] += samples[1][s] * m[1][6];
    buf[6] += samples[2][s] * m[2][6];
    buf[6] += samples[3][s] * m[3][6];
    buf[7]  = samples[0][s] * m[0][7];
    buf[7] += samples[1][s] * m[1][7];
    buf[7] += samples[2][s] * m[2][7];
    buf[7] += samples[3][s] * m[3][7];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
    samples[7][s] = buf[7];
  }
}

void Mixer::ip_mix51(samples_t samples, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    samples[0][s] = buf[0];
  }
}

void Mixer::ip_mix52(samples_t samples, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
  }
}

void Mixer::ip_mix53(samples_t samples, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
  }
}

void Mixer::ip_mix54(samples_t samples, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
  }
}

void Mixer::ip_mix55(samples_t samples, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
  }
}

void Mixer::ip_mix56(samples_t samples, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    buf[5] += samples[4][s] * m[4][5];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
  }
}

void Mixer::ip_mix57(samples_t samples, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    buf[5] += samples[4][s] * m[4][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[6] += samples[1][s] * m[1][6];
    buf[6] += samples[2][s] * m[2][6];
    buf[6] += samples[3][s] * m[3][6];
    buf[6] += samples[4][s] * m[4][6];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
  }
}

void Mixer::ip_mix58(samples_t samples, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    buf[5] += samples[4][s] * m[4][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[6] += samples[1][s] * m[1][6];
    buf[6] += samples[2][s] * m[2][6];
    buf[6] += samples[3][s] * m[3][6];
    buf[6] += samples[4][s] * m[4][6];
    buf[7]  = samples[0][s] * m[0][7];
    buf[7] += samples[1][s] * m[1][7];
    buf[7] += samples[2][s] * m[2][7];
    buf[7] += samples[3][s] * m[3][7];
    buf[7] += samples[4][s] * m[4][7];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
    samples[7][s] = buf[7];
  }
}

void Mixer::ip_mix61(samples_t samples, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    samples[0][s] = buf[0];
  }
}

void Mixer::ip_mix62(samples_t samples, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
  }
}

void Mixer::ip_mix63(samples_t samples, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
  }
}

void Mixer::ip_mix64(samples_t samples, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
  }
}

void Mixer::ip_mix65(samples_t samples, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[4] += samples[5][s] * m[5][4];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
  }
}

void Mixer::ip_mix66(samples_t samples, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[4] += samples[5][s] * m[5][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    buf[5] += samples[4][s] * m[4][5];
    buf[5] += samples[5][s] * m[5][5];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
  }
}

void Mixer::ip_mix67(samples_t samples, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[4] += samples[5][s] * m[5][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    buf[5] += samples[4][s] * m[4][5];
    buf[5] += samples[5][s] * m[5][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[6] += samples[1][s] * m[1][6];
    buf[6] += samples[2][s] * m[2][6];
    buf[6] += samples[3][s] * m[3][6];
    buf[6] += samples[4][s] * m[4][6];
    buf[6] += samples[5][s] * m[5][6];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
  }
}

void Mixer::ip_mix68(samples_t samples, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[4] += samples[5][s] * m[5][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    buf[5] += samples[4][s] * m[4][5];
    buf[5] += samples[5][s] * m[5][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[6] += samples[1][s] * m[1][6];
    buf[6] += samples[2][s] * m[2][6];
    buf[6] += samples[3][s] * m[3][6];
    buf[6] += samples[4][s] * m[4][6];
    buf[6] += samples[5][s] * m[5][6];
    buf[7]  = samples[0][s] * m[0][7];
    buf[7] += samples[1][s] * m[1][7];
    buf[7] += samples[2][s] * m[2][7];
    buf[7] += samples[3][s] * m[3][7];
    buf[7] += samples[4][s] * m[4][7];
    buf[7] += samples[5][s] * m[5][7];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
    samples[7][s] = buf[7];
  }
}

void Mixer::ip_mix71(samples_t samples, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    samples[0][s] = buf[0];
  }
}

void Mixer::ip_mix72(samples_t samples, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[1] += samples[6][s] * m[6][1];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
  }
}

void Mixer::ip_mix73(samples_t samples, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[1] += samples[6][s] * m[6][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[2] += samples[6][s] * m[6][2];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
  }
}

void Mixer::ip_mix74(samples_t samples, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[1] += samples[6][s] * m[6][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[2] += samples[6][s] * m[6][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    buf[3] += samples[6][s] * m[6][3];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
  }
}

void Mixer::ip_mix75(samples_t samples, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[1] += samples[6][s] * m[6][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[2] += samples[6][s] * m[6][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    buf[3] += samples[6][s] * m[6][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[4] += samples[5][s] * m[5][4];
    buf[4] += samples[6][s] * m[6][4];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
  }
}

void Mixer::ip_mix76(samples_t samples, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[1] += samples[6][s] * m[6][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[2] += samples[6][s] * m[6][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    buf[3] += samples[6][s] * m[6][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[4] += samples[5][s] * m[5][4];
    buf[4] += samples[6][s] * m[6][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    buf[5] += samples[4][s] * m[4][5];
    buf[5] += samples[5][s] * m[5][5];
    buf[5] += samples[6][s] * m[6][5];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
  }
}

void Mixer::ip_mix77(samples_t samples, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[1] += samples[6][s] * m[6][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[2] += samples[6][s] * m[6][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    buf[3] += samples[6][s] * m[6][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[4] += samples[5][s] * m[5][4];
    buf[4] += samples[6][s] * m[6][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    buf[5] += samples[4][s] * m[4][5];
    buf[5] += samples[5][s] * m[5][5];
    buf[5] += samples[6][s] * m[6][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[6] += samples[1][s] * m[1][6];
    buf[6] += samples[2][s] * m[2][6];
    buf[6] += samples[3][s] * m[3][6];
    buf[6] += samples[4][s] * m[4][6];
    buf[6] += samples[5][s] * m[5][6];
    buf[6] += samples[6][s] * m[6][6];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
  }
}

void Mixer::ip_mix78(samples_t samples, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[1] += samples[6][s] * m[6][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[2] += samples[6][s] * m[6][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    buf[3] += samples[6][s] * m[6][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[4] += samples[5][s] * m[5][4];
    buf[4] += samples[6][s] * m[6][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    buf[5] += samples[4][s] * m[4][5];
    buf[5] += samples[5][s] * m[5][5];
    buf[5] += samples[6][s] * m[6][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[6] += samples[1][s] * m[1][6];
    buf[6] += samples[2][s] * m[2][6];
    buf[6] += samples[3][s] * m[3][6];
    buf[6] += samples[4][s] * m[4][6];
    buf[6] += samples[5][s] * m[5][6];
    buf[6] += samples[6][s] * m[6][6];
    buf[7]  = samples[0][s] * m[0][7];
    buf[7] += samples[1][s] * m[1][7];
    buf[7] += samples[2][s] * m[2][7];
    buf[7] += samples[3][s] * m[3][7];
    buf[7] += samples[4][s] * m[4][7];
    buf[7] += samples[5][s] * m[5][7];
    buf[7] += samples[6][s] * m[6][7];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
    samples[7][s] = buf[7];
  }
}

void Mixer::ip_mix81(samples_t samples, size_t nsamples)
{
  sample_t buf[1];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[0] += samples[7][s] * m[7][0];
    samples[0][s] = buf[0];
  }
}

void Mixer::ip_mix82(samples_t samples, size_t nsamples)
{
  sample_t buf[2];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[0] += samples[7][s] * m[7][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[1] += samples[6][s] * m[6][1];
    buf[1] += samples[7][s] * m[7][1];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
  }
}

void Mixer::ip_mix83(samples_t samples, size_t nsamples)
{
  sample_t buf[3];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[0] += samples[7][s] * m[7][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[1] += samples[6][s] * m[6][1];
    buf[1] += samples[7][s] * m[7][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[2] += samples[6][s] * m[6][2];
    buf[2] += samples[7][s] * m[7][2];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
  }
}

void Mixer::ip_mix84(samples_t samples, size_t nsamples)
{
  sample_t buf[4];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[0] += samples[7][s] * m[7][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[1] += samples[6][s] * m[6][1];
    buf[1] += samples[7][s] * m[7][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[2] += samples[6][s] * m[6][2];
    buf[2] += samples[7][s] * m[7][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    buf[3] += samples[6][s] * m[6][3];
    buf[3] += samples[7][s] * m[7][3];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
  }
}

void Mixer::ip_mix85(samples_t samples, size_t nsamples)
{
  sample_t buf[5];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[0] += samples[7][s] * m[7][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[1] += samples[6][s] * m[6][1];
    buf[1] += samples[7][s] * m[7][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[2] += samples[6][s] * m[6][2];
    buf[2] += samples[7][s] * m[7][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    buf[3] += samples[6][s] * m[6][3];
    buf[3] += samples[7][s] * m[7][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[4] += samples[5][s] * m[5][4];
    buf[4] += samples[6][s] * m[6][4];
    buf[4] += samples[7][s] * m[7][4];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
  }
}

void Mixer::ip_mix86(samples_t samples, size_t nsamples)
{
  sample_t buf[6];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[0] += samples[7][s] * m[7][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[1] += samples[6][s] * m[6][1];
    buf[1] += samples[7][s] * m[7][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[2] += samples[6][s] * m[6][2];
    buf[2] += samples[7][s] * m[7][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    buf[3] += samples[6][s] * m[6][3];
    buf[3] += samples[7][s] * m[7][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[4] += samples[5][s] * m[5][4];
    buf[4] += samples[6][s] * m[6][4];
    buf[4] += samples[7][s] * m[7][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    buf[5] += samples[4][s] * m[4][5];
    buf[5] += samples[5][s] * m[5][5];
    buf[5] += samples[6][s] * m[6][5];
    buf[5] += samples[7][s] * m[7][5];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
  }
}

void Mixer::ip_mix87(samples_t samples, size_t nsamples)
{
  sample_t buf[7];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[0] += samples[7][s] * m[7][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[1] += samples[6][s] * m[6][1];
    buf[1] += samples[7][s] * m[7][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[2] += samples[6][s] * m[6][2];
    buf[2] += samples[7][s] * m[7][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    buf[3] += samples[6][s] * m[6][3];
    buf[3] += samples[7][s] * m[7][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[4] += samples[5][s] * m[5][4];
    buf[4] += samples[6][s] * m[6][4];
    buf[4] += samples[7][s] * m[7][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    buf[5] += samples[4][s] * m[4][5];
    buf[5] += samples[5][s] * m[5][5];
    buf[5] += samples[6][s] * m[6][5];
    buf[5] += samples[7][s] * m[7][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[6] += samples[1][s] * m[1][6];
    buf[6] += samples[2][s] * m[2][6];
    buf[6] += samples[3][s] * m[3][6];
    buf[6] += samples[4][s] * m[4][6];
    buf[6] += samples[5][s] * m[5][6];
    buf[6] += samples[6][s] * m[6][6];
    buf[6] += samples[7][s] * m[7][6];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
  }
}

void Mixer::ip_mix88(samples_t samples, size_t nsamples)
{
  sample_t buf[8];
  for (size_t s = 0; s < nsamples; s++)
  {
    buf[0]  = samples[0][s] * m[0][0];
    buf[0] += samples[1][s] * m[1][0];
    buf[0] += samples[2][s] * m[2][0];
    buf[0] += samples[3][s] * m[3][0];
    buf[0] += samples[4][s] * m[4][0];
    buf[0] += samples[5][s] * m[5][0];
    buf[0] += samples[6][s] * m[6][0];
    buf[0] += samples[7][s] * m[7][0];
    buf[1]  = samples[0][s] * m[0][1];
    buf[1] += samples[1][s] * m[1][1];
    buf[1] += samples[2][s] * m[2][1];
    buf[1] += samples[3][s] * m[3][1];
    buf[1] += samples[4][s] * m[4][1];
    buf[1] += samples[5][s] * m[5][1];
    buf[1] += samples[6][s] * m[6][1];
    buf[1] += samples[7][s] * m[7][1];
    buf[2]  = samples[0][s] * m[0][2];
    buf[2] += samples[1][s] * m[1][2];
    buf[2] += samples[2][s] * m[2][2];
    buf[2] += samples[3][s] * m[3][2];
    buf[2] += samples[4][s] * m[4][2];
    buf[2] += samples[5][s] * m[5][2];
    buf[2] += samples[6][s] * m[6][2];
    buf[2] += samples[7][s] * m[7][2];
    buf[3]  = samples[0][s] * m[0][3];
    buf[3] += samples[1][s] * m[1][3];
    buf[3] += samples[2][s] * m[2][3];
    buf[3] += samples[3][s] * m[3][3];
    buf[3] += samples[4][s] * m[4][3];
    buf[3] += samples[5][s] * m[5][3];
    buf[3] += samples[6][s] * m[6][3];
    buf[3] += samples[7][s] * m[7][3];
    buf[4]  = samples[0][s] * m[0][4];
    buf[4] += samples[1][s] * m[1][4];
    buf[4] += samples[2][s] * m[2][4];
    buf[4] += samples[3][s] * m[3][4];
    buf[4] += samples[4][s] * m[4][4];
    buf[4] += samples[5][s] * m[5][4];
    buf[4] += samples[6][s] * m[6][4];
    buf[4] += samples[7][s] * m[7][4];
    buf[5]  = samples[0][s] * m[0][5];
    buf[5] += samples[1][s] * m[1][5];
    buf[5] += samples[2][s] * m[2][5];
    buf[5] += samples[3][s] * m[3][5];
    buf[5] += samples[4][s] * m[4][5];
    buf[5] += samples[5][s] * m[5][5];
    buf[5] += samples[6][s] * m[6][5];
    buf[5] += samples[7][s] * m[7][5];
    buf[6]  = samples[0][s] * m[0][6];
    buf[6] += samples[1][s] * m[1][6];
    buf[6] += samples[2][s] * m[2][6];
    buf[6] += samples[3][s] * m[3][6];
    buf[6] += samples[4][s] * m[4][6];
    buf[6] += samples[5][s] * m[5][6];
    buf[6] += samples[6][s] * m[6][6];
    buf[6] += samples[7][s] * m[7][6];
    buf[7]  = samples[0][s] * m[0][7];
    buf[7] += samples[1][s] * m[1][7];
    buf[7] += samples[2][s] * m[2][7];
    buf[7] += samples[3][s] * m[3][7];
    buf[7] += samples[4][s] * m[4][7];
    buf[7] += samples[5][s] * m[5][7];
    buf[7] += samples[6][s] * m[6][7];
    buf[7] += samples[7][s] * m[7][7];
    samples[0][s] = buf[0];
    samples[1][s] = buf[1];
    samples[2][s] = buf[2];
    samples[3][s] = buf[3];
    samples[4][s] = buf[4];
    samples[5][s] = buf[5];
    samples[6][s] = buf[6];
    samples[7][s] = buf[7];
  }
}

