#include "sink.h"

class SinkThunk : public Sink
{
protected:
  Sink2 *s;
  Chunk2 in_chunk;
  Speakers spk;

public:
  SinkThunk(Sink2 *f);

  virtual bool query_input(Speakers new_spk) const
  { return s->can_open(new_spk); }

  virtual bool set_input(Speakers new_spk)
  {
    if (s->open(new_spk))
      spk = new_spk;
  }

  virtual Speakers get_input() const
  { return spk; }

  virtual bool process(const Chunk *chunk)
  {
    if (!chunk || chunk->is_dummy())
      return true;

    if (chunk->spk != spk)
    {
      s->flush();
      if (!s->open(chunk->spk))
        return false;
    }

    s->process(Chunk2(*chunk));
    return true;
  }
};


Sink2::Sink2()
{
  thunk = new SinkThunk(this);
}

Sink2::~Sink2()
{
  safe_delete(thunk);
}
