#include "source.h"

class SourceThunk : public Source
{
protected:
  Source2 *s;
  Speakers spk;
  mutable Chunk out_chunk;
  mutable bool processing;
  mutable bool new_stream;

public:
  SourceThunk(Source2 *source):
  s(source), processing(true), new_stream(false)
  {}

  void reset()
  {
    processing = false;
    new_stream = false;
    out_chunk.set_dummy();
  }

  virtual Speakers get_output() const
  { return spk; }

  virtual bool is_empty() const
  {
    if (processing || new_stream || !out_chunk.is_dummy())
      return false;

    Chunk2 out_chunk2;
    processing = s->get_chunk(out_chunk2);
    if (processing)
    {
      out_chunk.set(s->get_output(),
        out_chunk2.rawdata, out_chunk2.samples, out_chunk2.size,
        out_chunk2.sync, out_chunk2.time);
      return false;
    }
    return true;
  }

  virtual bool get_chunk(Chunk *chunk)
  {
    if (!out_chunk.is_dummy())
    {
      *chunk = out_chunk;
      out_chunk.set_dummy();
      return true;
    }

    Chunk2 out_chunk2;
    processing = s->get_chunk(out_chunk2);
    if (processing)
    {
      out_chunk.set(s->get_output(),
        out_chunk2.rawdata, out_chunk2.samples, out_chunk2.size,
        out_chunk2.sync, out_chunk2.time);

      new_stream = s->new_stream();
      if (!new_stream || spk.is_unknown())
        spk = s->get_output();

      if (new_stream)
      {
        chunk->set_empty(spk);
        chunk->set_eos(true);
        spk = s->get_output();
        new_stream = false;
        return true;
      }

      *chunk = out_chunk;
      out_chunk.set_dummy();
      return true;
    }
    else
    {
      out_chunk.set_dummy();
      chunk->set_empty(spk);
      chunk->set_eos();
      return true;
    }
  }
};


Source2::Source2()
{
  thunk = new SourceThunk(this);
}

Source2::~Source2()
{
  safe_delete(thunk);
}

void
Source2::reset_thunk()
{
  if (thunk)
    dynamic_cast<SourceThunk*>(thunk)->reset();
}

string
Source2::name() const
{
  string type_name = typeid(*this).name();
  if (type_name.compare(0, 6, "class ") == 0)
    type_name.replace(0, 6, "");
  return type_name;
}
