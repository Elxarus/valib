#include "list_source.h"

void
ListSource::set(Speakers spk, const Chunk *chunks, size_t count)
{
  start_spk = spk;
  list.resize(count);
  for (size_t i = 0; i < count; i++)
    list[i].chunk = chunks[i];
  reset();
}

void
ListSource::set(Speakers spk, const chunk_list_t &chunks)
{
  start_spk = spk;
  list.resize(chunks.size());
  for (size_t i = 0; i < chunks.size(); i++)
    list[i].chunk = chunks[i];
  reset();
}

void
ListSource::set(Speakers spk, const FormatChangeChunk *chunks, size_t count)
{
  start_spk = spk;
  list.resize(count);
  for (size_t i = 0; i < count; i++)
  {
    list[i] = chunks[i];
    if (list[i].new_stream)
      assert(!list[i].spk.is_unknown());
  }
  reset();
}

void
ListSource::set(Speakers spk, const fchunk_list_t &chunks)
{
  start_spk = spk;
  list = chunks;

  for (size_t i = 0; i < list.size(); i++)
    if (list[i].new_stream)
      assert(!list[i].spk.is_unknown());
  reset();
}

/////////////////////////////////////////////////////////
// Source interface

bool
ListSource::get_chunk(Chunk &out)
{
  if (pos >= list.size())
    return false;

  out = list[pos].chunk;
  if (list[pos].new_stream)
    out_spk = list[pos].spk;
  is_new_stream = list[pos].new_stream;
  pos++;

  return true;
}

void
ListSource::reset()
{
  pos = 0;
  out_spk = start_spk;
  is_new_stream = false;
}

bool
ListSource::new_stream() const
{
  return is_new_stream;
}

Speakers
ListSource::get_output() const
{
  return out_spk;
}
