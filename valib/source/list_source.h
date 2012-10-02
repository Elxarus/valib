/**************************************************************************//**
  \file list_source.h
  \brief ListSource class
******************************************************************************/


#ifndef VALIB_LIST_SOURCE_H
#define VALIB_LIST_SOURCE_H

#include <vector>
#include "../source.h"

/**************************************************************************//**
  \class FormatChangeChunk
  \brief Complete output state of a filter or source.

  In some cases it is nessesary to hold a complete output state of a source or
  a filter. This structure extends Chunk to hold format change information.

  \var Chunk FormatChangeChunk::chunk;
    Chunk returned by Source::get_chunk() or Filter::process().

  \var bool FormatChangeChunk::new_stream;
    Flag returned by Source::new_stream() or Filter::new_stream().

  \var Speakers FormatChangeChunk::spk;
    Output format of a filter. This field must be set when new_stream is true.
    When new_stream is false this field may be set to either FORMAT_UNKNOWN or
    correct output format of a filter or a sink.
******************************************************************************/

struct FormatChangeChunk
{
  Chunk    chunk;
  bool     new_stream;
  Speakers spk;

  FormatChangeChunk():
    new_stream(false)
  {}

  FormatChangeChunk(const Chunk &chunk_, bool new_stream_ = false, Speakers spk_ = Speakers()):
    chunk(chunk_),
    new_stream(new_stream_),
    spk(spk_)
  {}
};

/**************************************************************************//**
  \class ListSource
  \brief Given a plain list of chunks this class outputs it as a Source.
******************************************************************************/

class ListSource: public Source
{
public:
  typedef std::vector<Chunk> chunk_list_t;
  typedef std::vector<FormatChangeChunk> fchunk_list_t;

  ListSource(): pos(0), is_new_stream(false)
  {}

  ListSource(Speakers spk, const Chunk *chunks, size_t count)
  { set(spk, chunks, count); }

  ListSource(Speakers spk, const chunk_list_t &chunks)
  { set(spk, chunks); }

  ListSource(Speakers spk, const FormatChangeChunk *chunks, size_t count)
  { set(spk, chunks, count); }

  ListSource(Speakers spk, const fchunk_list_t &chunks)
  { set(spk, chunks); }
 
  /////////////////////////////////////////////////////////
  // ListSource interface

  void set(Speakers spk, const Chunk *chunks, size_t count);
  void set(Speakers spk, const chunk_list_t &chunks);
  void set(Speakers spk, const FormatChangeChunk *chunks, size_t count);
  void set(Speakers spk, const fchunk_list_t &chunks);

  inline fchunk_list_t get_list() const
  { return list; }

  inline size_t get_pos() const
  { return pos; }

  /////////////////////////////////////////////////////////
  // Source interface

  virtual void reset();
  virtual bool get_chunk(Chunk &out);
  virtual bool new_stream() const;
  virtual Speakers get_output() const;

protected:
  fchunk_list_t list; //<! List of chunks
  size_t pos;         //<! Current chunk

  Speakers start_spk; //<! Initial output format
  Speakers out_spk;   //<! Current output format
  bool is_new_stream; //<! New stream flag
};

#endif
