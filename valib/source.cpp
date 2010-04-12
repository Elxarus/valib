#include "source.h"

class SourceThunk : public Source
{
protected:
  Source2 *s;
  Speakers spk;
  Chunk out_chunk;

  bool processing;
  bool new_stream;

public:
  SourceThunk(Source2 *source):
  s(source), processing(true), new_stream(false)
  {}

  virtual Speakers get_output() const
  { return spk; }

  virtual bool is_empty() const
  { return !processing && !new_stream && out_chunk.is_dummy(); }

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

string
Source2::name() const
{
  string type_name = typeid(*this).name();
  if (type_name.compare(0, 6, "class ") == 0)
    type_name.replace(0, 6, "");
  return type_name;
}
