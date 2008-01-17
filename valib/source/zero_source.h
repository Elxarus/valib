#ifndef VALIB_ZERO_SOURCE_H
#define VALIB_ZERO_SOURCE_H

#include "../filter.h"
#include "../data.h"

/*
  ZeroSource class is source that generates blocks of zero data.
*/


class ZeroSource : public Source
{
protected:
  Speakers  spk;
  SampleBuf buf;
  size_t    buf_size;
  size_t    data_size;

public:
  ZeroSource();
  ZeroSource(Speakers spk, size_t data_size, size_t buf_size = 4096);

  bool   set_output(Speakers spk, size_t data_size, size_t buf_size = 4096);
  size_t get_data_size();
  void   set_data_size(size_t _data_size);

  // Source interface
  virtual Speakers get_output() const;
  virtual bool is_empty() const;
  virtual bool get_chunk(Chunk *chunk);
};

#endif
