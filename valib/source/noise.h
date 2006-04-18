// byteorder-dependent: Noise::get_chunk()

#ifndef VALIB_NOISE_H
#define VALIB_NOISE_H

#include "filter.h"
#include "rng.h"
#include "data.h"

/*
  Noise class is source that generates blocks of noise data.

  For raw formats it generates binary noise blocks. It is acceptable for
  integer PCM formats (either signed/unsigned and low/big endians),
  compressed formats (parser shoud not crash in this conditions), etc.
  But it is not for floating-point PCM formats! FP PCM noise shoud be limited
  with -1...1 range (otherwise it is senseless), and binary noise does not
  represent correct FP PCM noise.

  For linear format it generates correct noise blocks.

  Output is generated as sequence of data blocks with last block marked 
  with eos. Total data size to output and size of blocks may be specified.
  Number of output blocks are defined as ceil(data_size/block_size), so
  last block may be short.

  It may generate blocks of random size. Negative value for block_size
  means that blocks of random size up to abs(block_size) should be generated.

  This class allocates memory for data block.

  bool set_output(Speakers spk, size_t data_size = 65536, size_t buf_size = 4096)
    Sets generation parameters:
    * spk       - output data format
    * data_size - total amount of data to output
    * buf_size  - output block size

  void set_seed(long seed)
    Sets generator's seed

  size_t get_data_size();
    Returms amount of data remains to output.

  void set_data_size(size_t _data_size)
    Allows to change amount of data to output.
*/


class Noise : public Source
{
protected:
  RNG rng;

  Speakers  spk;
  SampleBuf buf;
  int       buf_size;
  size_t    data_size;

public:
  Noise();
  Noise(Speakers spk, size_t data_size, int buf_size = 4096);

  bool   set_output(Speakers spk, size_t data_size = 65536, int buf_size = 4096);
  void   set_seed(long seed);
  size_t get_data_size();
  void   set_data_size(size_t _data_size);

  // Source interface
  virtual Speakers get_output() const;
  virtual bool is_empty() const;
  virtual bool get_chunk(Chunk *chunk);
};

#endif
