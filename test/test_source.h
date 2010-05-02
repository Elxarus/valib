/*
  Simple class that acts as file source when filename is specified and as
  noise source otherwise. Useful as test source because not all formats
  require a special file to test.
*/

#ifndef TEST_SOURCE_H
#define TEST_SOURCE_H

#include "source\raw_source.h"
#include "source\generator.h"

class TestSource: public Source2
{
protected:
  const char *filename;

  RAWSource   file;   // file source
  NoiseGen    noise;  // noise source
  Source2    *source; // current source

public:
  TestSource()
  {
    filename = 0;
    source = 0;
  }

  bool open(Speakers spk_, const char *filename_, size_t block_size_)
  {
    filename = filename_;
    if (filename_)
    {
      source = &file;
      return file.open(spk_, filename_, block_size_);
    }
    else
    {
      source = &noise;
      noise.init(spk_, 8575, MAX(10*block_size_, 65536), block_size_);
      return true;
    }
  }

  bool is_open()
  {
    if (filename)
      return file.is_open();
    else
      return source != 0;
  }

  virtual void reset()
  { reset_thunk(); }

  virtual bool get_chunk(Chunk2 &out)
  { return source? source->get_chunk(out): false; }

  virtual bool new_stream() const
  { return source? source->new_stream(): false; }

  virtual Speakers get_output() const
  { return source? source->get_output(): spk_unknown; }
};

#endif
