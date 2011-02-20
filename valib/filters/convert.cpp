#include <math.h>
#include "convert.h"

// todo: PCM-to-PCM conversions

static const int converter_formats = FORMAT_MASK_LINEAR | FORMAT_MASK_PCM16 | FORMAT_MASK_PCM24 | FORMAT_MASK_PCM32 | FORMAT_MASK_PCM16_BE | FORMAT_MASK_PCM24_BE | FORMAT_MASK_PCM32_BE | FORMAT_MASK_PCMFLOAT | FORMAT_MASK_PCMDOUBLE | FORMAT_MASK_LPCM20 | FORMAT_MASK_LPCM24;

Converter::Converter(size_t _nsamples)
{
  convert = 0;
  format = FORMAT_UNKNOWN;
  memcpy(order, std_order, sizeof(order));
  nsamples = _nsamples;
  part_size = 0;
}

convert_t 
Converter::find_conversion(int _format, Speakers _spk) const
{
  if (_format == FORMAT_LINEAR)
    return find_pcm2linear(_spk.format, _spk.nch());
  else if (_spk.format == FORMAT_LINEAR)
    return find_linear2pcm(_format, _spk.nch());

  return 0;
}

bool
Converter::convert_pcm2linear(Chunk &in, Chunk &out)
{
  const size_t sample_size = spk.sample_size() * spk.nch();
  uint8_t *rawdata  = in.rawdata;
  size_t   size     = in.size;
  size_t   out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of a sample

  if (part_size)
  {
    assert(part_size < sample_size);
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      in.clear();
      return false;
    }
    else
    {
      // fill the buffer & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      part_size = 0;
      rawdata += delta;
      size -= delta;

      convert(part_buf, out_samples, 1);

      out_size++;
      if (is_lpcm(spk.format))
        out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Determine the number of samples to convert and
  // the size of raw data required for conversion
  // Note: LPCM sample size is doubled because one block
  // contains 2 samples. Also, decoding is done in 2 sample
  // blocks, thus we have to specify less cycles.

  size_t n = nsamples - out_size;
  if (is_lpcm(spk.format)) n /= 2;
  size_t n_size = n * sample_size;
  if (n_size > size)
  {
    n = size / sample_size;
    n_size = n * sample_size;
  }

  /////////////////////////////////////////////////////////
  // Convert

  convert(rawdata, out_samples + out_size, n);

  out_size += n;
  if (is_lpcm(spk.format))
    out_size += n;

  rawdata += n_size;
  size -= n_size;

  /////////////////////////////////////////////////////////
  // Remember the remaining part of a sample

  if (size && size < sample_size)
  {
    memcpy(part_buf, rawdata, size);
    part_size = size;
    rawdata += size;
    size = 0;
  }

  /////////////////////////////////////////////////////////
  // Make output chunk and update input

  out.set_linear(out_samples, out_size, in.sync, in.time);
  out.samples.reorder_to_std(spk, order);
  in.set_rawdata(rawdata, size);
  return !out.is_dummy();
}

bool
Converter::convert_linear2pcm(Chunk &in, Chunk &out)
{
  size_t n = MIN(in.size, nsamples);
  size_t out_size = n * sample_size(format) * spk.nch();

  samples_t samples = in.samples;
  samples.reorder_from_std(spk, order);

  convert(out_rawdata, samples, n);

  out.set_rawdata(out_rawdata, out_size, in.sync, in.time);
  in.drop_samples(n);
  return !out.is_dummy();
}

///////////////////////////////////////////////////////////
// Converter interface

size_t 
Converter::get_buffer() const
{
  return nsamples;
}

bool 
Converter::set_buffer(size_t _nsamples)
{
  nsamples = _nsamples;
  if (is_open())
    return open(spk);
  return true;
}

int 
Converter::get_format() const
{
  return format;
}

bool 
Converter::set_format(int _format)
{
  if ((FORMAT_MASK(_format) & converter_formats) == 0)
    return false;

  format = _format;
  if (is_open())
    return open(spk);
  return true;
}

void 
Converter::get_order(order_t _order) const
{
  memcpy(_order, order, sizeof(order));
}

void 
Converter::set_order(const order_t _order)
{
  memcpy(order, _order, sizeof(order));
}

///////////////////////////////////////////////////////////
// Filter interface

bool 
Converter::can_open(Speakers new_spk) const
{
  if ((FORMAT_MASK(new_spk.format) & converter_formats) == 0)
    return false;

  if (new_spk.format == format)
    return true; 

  if (new_spk.nch() == 0)
    return false;

  if (find_conversion(format, new_spk) == 0)
    return false;

  return true;
}

bool 
Converter::init()
{
  /////////////////////////////////////////////////////////
  // Initialize convertor:
  // * find conversion function
  // * allocate buffer
  // * reset filter state

  if (spk.format == format)
    // no conversion required; no buffer required
    return true; 
  
  convert = find_conversion(format, spk);
  if (convert == 0)
    return false;

  buf.allocate(spk.nch() * nsamples * sample_size(format));

  if (format == FORMAT_LINEAR)
  {
    // set channel pointers
    out_samples[0] = (sample_t *)buf.begin();
    for (int ch = 1; ch < spk.nch(); ch++)
      out_samples[ch] = out_samples[ch-1] + nsamples;
    out_rawdata = 0;
  }
  else
  {
    // set rawdata pointer
    out_rawdata = buf.begin();
    out_samples.zero();
  }

  reset();
  return true;
}

Speakers 
Converter::get_output() const
{
  if (spk.format == format)
    return spk;

  if (convert == 0)
    return spk_unknown;

  Speakers out = spk;
  out.format = format;
  return out;
}

bool 
Converter::process(Chunk &in, Chunk &out)
{
  if (spk.format == format)
  {
    out = in;
    in.clear();
    return !out.is_dummy();
  }

  assert(convert != 0);

  if (format == FORMAT_LINEAR)
    return convert_pcm2linear(in, out);
  else
    return convert_linear2pcm(in, out);
}

void
Converter::reset()
{
  part_size = 0;
}
