#include <stdio.h>
#include "decoder.h"

/*

This filter is data-dependent. Therefore it must follow format change rules
carefully.

Initial state when output format is not known is called transition state.
Also we may switch to transition state when we loose syncrinization and after
flushing.

When frame was loaded filter must know output format after process() call. 
Therefore process() must decode the frame after successful frame load. To
avoid unnessesary dummy output get_chunk() should load a new frame immediately
after output of current one. Therefore it is 2 special full states:
frame_decoded and frame_loaded. frame_decoded state is used only after
successful syncronization in process() call. frame_loaded state means that we
have unparsed frame loaded at stream buffer.

Filter must flush output on stream change. To correctly finish the stream
format_change state is used.

Also filter must flush after the end of input stream, flushing state is used
in this case. But note that we should not do excessive flushing. I.e. if stream
was already finished with inter-stream flushing and new stream was not started
we should not flush, but must reset the filter. So we should consider direct
path to transition state without going through flushing state.



States list
===========

Decoder state   Filter state
----------------------------
transition      transition
frame_decoded   full
frame_loaded    full
no_data         empty
format_change   full
flushing        flushing



State transitions
=================

----------
transition
----------

In this state we do not know output format because we have no enough data to
determine it. So either stream buffer did not catch a syncronization or it was
an error during decoding. Filter is empty in this state and output format is 
unknown.

process() call tries to syncronize, load and decode a new frame.

from state      to state        function call   transition description
-----------------------------------------------------------------------------
transition      transition      process()       not enough data for sync; reset the filter if input stream is ended (flushing flag is set)
transition      frame_decoded   process()       we have the sync catched and a frame decoded; output format changes to a new one
transition      frame_loaded    -               frame is decoded after process() call
transition      no_data         -               we must start a stream before
transition      format_change   -               we must start a stream before
transition      flushing        -               we must start a stream before

-------------
frame_decoded
-------------

In this state stream buffer has a frame loaded and the parser have it decoded.
Filter is full and output format equals to the parser format.

get_chunk() outputs the decoded frame and then tries to load a new frame 
(without decoding)

from state      to state        function call   transition description
-----------------------------------------------------------------------------
frame_decoded   transition      -               we can get into transition state only after flushing
frame_decoded   frame_decoded   -               we cannot decode a new frame because it will break current output
frame_decoded   frame_loaded    get_chunk()     new frame was loaded and it belongs to the same stream
frame_decoded   no_data         get_chunk()     new frame was not loaded (not enough data) and input stream is not ended
frame_decoded   format_change   get_chunk()     new frame was loaded and it belongs to a new stream
frame_decoded   flushing        get_chunk()     new frame was not loaded and input stream is ended

------------
frame_loaded
------------
                    
In this state stream buffer has a frame loaded. Filter is full and output
format equals to the old parser format.

get_chunk() decodes and outputs the decoded frame (or a dummy if decoding fails)
and then tries to load a new frame (without decoding)

from state      to state        function call   transition description
-----------------------------------------------------------------------------
frame_loaded    transition      -               we can get into transition state only after flushing
frame_loaded    frame_decoded   -               we cannot decode a new frame because it will break current output
frame_loaded    frame_loaded    get_chunk()     new frame was loaded and it belongs to the same stream
frame_loaded    no_data         get_chunk()     new frame was not loaded (not enough data) and input stream is not ended
frame_loaded    format_change   get_chunk()     new frame was loaded and it belongs to a new stream
frame_loaded    flushing        get_chunk()     new frame was not loaded and input stream is ended

-------
no_data
-------

In this state stream buffer has only a part of a frame loaded. Filter is empty
and output format equals to the old parser format.

process() tries to load and decode a frame.                   

from state      to state        function call   transition description
-----------------------------------------------------------------------------
no_data         transition      -               we can get into transition state only after flushing
no_data         frame_decoded   -               do not decode the frame to avoid excessive decoding on format change
no_data         frame_loaded    process()       new frame was loaded and it belongs to the same stream
no_data         no_data         process()       new frame was not loaded (not enough data) and input stream is not ended
no_data         format_change   process()       new frame was loaded and it belongs to a new stream
no_data         flushing        process(eos)    new frame was not loaded and input stream is ended

-------------
format change
-------------

In this state stream buffer has a frame of a new format loaded. Filter if full
and output format equals to the old parser format.

get_chunk() flushes current output stream and tries to decode a new frame to
determine new output format.

from state      to state        function call   transition description
-----------------------------------------------------------------------------
format_change   transition      get_chunk(eos)  decoding was failed and next try to load and decode next frram was failed too, so we cannot determine new output format; output format changes to unknown; reset the filter if input stream is ended (flushing flag is set)
format_change   frame_decoded   get_chunk(eos)  new frame was decoded; output format changes to the new parser format
format_change   frame_loaded    -               new frame was decoded already
format_change   no_data         -               if we have no enough data we must go to transition state (output format is unknown)
format_change   format_change   -               new format change is senseless
format_change   flushing        -               stream was already finished

--------
flushing
--------
                        
In this state we must just end current output stream and reset the filter if
input stream is ended.

get_chunk() flushed current output stream and resets the filter if input stream
is ended (flushing flag is set).

from state      to state        function call   transition description
-----------------------------------------------------------------------------
flushing        transition      get_chunk(eos)  output format changes to unknown; reset the filter if input stream is ended (flushing flag is set)
flushing        sync_lost       -
flushing        frame_decoded   -
flushing        frame_loaded    -
flushing        no_data         -
flushing        format_change   -       
flushing        flushing        -



Transitions list
================

transition      transition      process()       not enough data for sync; reset the filter if input stream is ended (flushing flag is set)
transition      frame_decoded   process()       we have catched the sync and decoded a frame; output format changes to a new one
frame_decoded   frame_loaded    get_chunk()     new frame was loaded and it belongs to the same stream
frame_decoded   no_data         get_chunk()     new frame was not loaded (not enough data) and input stream is not ended
frame_decoded   format_change   get_chunk()     new frame was loaded and it belongs to a new stream
frame_decoded   flushing        get_chunk()     new frame was not loaded and input stream is ended
frame_loaded    frame_loaded    get_chunk()     new frame was loaded and it belongs to the same stream
frame_loaded    no_data         get_chunk()     new frame was not loaded (not enough data) and input stream is not ended
frame_loaded    format_change   get_chunk()     new frame was loaded and it belongs to a new stream
frame_loaded    flushing        get_chunk()     new frame was not loaded and input stream is ended
no_data         frame_loaded    process()       new frame was loaded and it belongs to the same stream
no_data         no_data         process()       new frame was not loaded (not enough data) and input stream is not ended
no_data         format_change   process()       new frame was loaded and it belongs to a new stream
no_data         flushing        process(eos)    new frame was not loaded and input stream is ended
format_change   transition      get_chunk(eos)  "decoding was failed and next try to load and decode next frram was failed too, so we cannot determine new output format; output format changes to unknown; reset the filter if input stream is ended (flushing flag is set)"
format_change   frame_decoded   get_chunk(eos)  new frame was decoded; output format changes to the new parser format
flushing        transition      get_chunk(eos)  output format changes to unknown; reset the filter if input stream is ended (flushing flag is set)

*/



Decoder::Decoder()
:NullFilter(-1)
{
  parser = 0;

  out_spk = spk_unknown;
  state = state_transition;
  new_stream = false;
}

Decoder::Decoder(FrameParser *_parser)
:NullFilter(-1)
{
  parser = 0;

  out_spk = spk_unknown;
  state = state_transition;
  new_stream = false;

  set_parser(_parser);
}

Decoder::~Decoder()
{
}

bool
Decoder::set_parser(FrameParser *_parser)
{
  reset();
  parser = 0;

  if (!_parser)
    return true;

  const HeaderParser *header_parser = _parser->header_parser();
  if (!stream.set_parser(header_parser))
    return false;

  parser = _parser;
  return true;
}

const FrameParser *
Decoder::get_parser() const
{
  return parser;
}



void
Decoder::reset()
{
  NullFilter::reset();

  out_spk = spk_unknown;
  state = state_transition;
  new_stream = false;

  if (parser)
    parser->reset();
  stream.reset();
  sync_helper.reset();
}

bool
Decoder::is_ofdd() const
{
  return true;
}

bool
Decoder::query_input(Speakers spk) const
{
  if (!parser) 
    return false;
  else if (spk.format == FORMAT_RAWDATA) 
    return true;
  else
    return parser->header_parser()->can_parse(spk.format);
}

bool
Decoder::process(const Chunk *_chunk)
{
  if (!parser)
    return false;

  // we must ignore dummy chunks
  if (_chunk->is_dummy())
    return true;

  // receive the chunk
  FILTER_SAFE(receive_chunk(_chunk));
  sync_helper.receive_sync(sync, time);
  sync = false;


  switch (state)
  {
    case state_transition:
    {
      if (load_decode_frame())
      {
        out_spk = parser->get_spk();
        state = state_frame_decoded;
        new_stream = false;
      }
      else
      {
        // if we did not start a stream we must forget about current stream on 
        // flushing and drop data currently buffered (flushing state is also
        // dropped so we do not pass eos event in this case)
        if (flushing)
          reset();
      }
      return true;
    }

    case state_no_data:
    {
      if (load_frame())
        state = new_stream? state_format_change: state_frame_loaded;
      else 
        state = flushing? state_flushing: state_no_data;
      return true;
    }

    default: 
      return false;
  }
}

Speakers
Decoder::get_output() const
{
  return out_spk;
}

bool
Decoder::is_empty() const
{
  return state == state_transition || state == state_no_data;
}

bool 
Decoder::get_chunk(Chunk *_chunk)
{
  if (!parser) 
    return false;

  switch (state)
  {
    case state_frame_decoded:
      // send the decoded frame
      if (out_spk.is_linear())
        _chunk->set_linear(out_spk, parser->get_samples(), parser->get_nsamples());
      else
        _chunk->set_rawdata(out_spk, parser->get_rawdata(), parser->get_rawsize());
      sync_helper.send_sync(_chunk);

      // load next frame
      if (load_frame())
        state = new_stream? state_format_change: state_frame_loaded;
      else
        state = flushing? state_flushing: state_no_data;
      return true;

    case state_frame_loaded:
      // decode and send the frame
      if (!decode_frame())
        _chunk->set_empty(out_spk);
      else if (out_spk.is_linear())
        _chunk->set_linear(out_spk, parser->get_samples(), parser->get_nsamples());
      else
        _chunk->set_rawdata(out_spk, parser->get_rawdata(), parser->get_rawsize());
      sync_helper.send_sync(_chunk);

      // load next frame
      if (load_frame())
        state = new_stream? state_format_change: state_frame_loaded;
      else
        state = flushing? state_flushing: state_no_data;
      return true;

    case state_format_change:
      // send flushing
      _chunk->set_empty(out_spk);
      _chunk->set_eos();

      // try to determine new output format
      if (decode_frame())
      {
        out_spk = parser->get_spk();
        state = state_frame_decoded;
      }
      else if (load_decode_frame())
      {
        out_spk = parser->get_spk();
        state = state_frame_decoded;
      }
      else
      {
        out_spk = spk_unknown;
        state = state_transition;
        if (flushing)
          reset();
      }

      new_stream = false;
      return true;

    case state_flushing:
      // send flushing and switch to transition state
      _chunk->set_empty(out_spk);
      _chunk->set_eos();

      out_spk = parser->get_spk();
      state = state_frame_decoded;
      if (flushing)
        reset();
      return true;

    default:
      return false;
  }
}

bool
Decoder::load_frame()
{
  uint8_t *end = rawdata + size;
  bool result = stream.load_frame(&rawdata, end);
  size = end - rawdata;
  new_stream |= stream.is_new_stream();
  return result;
}

bool
Decoder::decode_frame()
{
  return parser->parse_frame(stream.get_frame(), stream.get_frame_size());
}

bool
Decoder::load_decode_frame()
{
  uint8_t *end = rawdata + size;
  while (stream.load_frame(&rawdata, end))
  {
    new_stream |= stream.is_new_stream();
    if (parser->parse_frame(stream.get_frame(), stream.get_frame_size()))
    {
      size = end - rawdata;
      return true;
    }
  }

  size = 0;
  return false;
}


AudioDecoder::AudioDecoder()
:NullFilter(FORMAT_MASK_MPA | FORMAT_MASK_AC3 | FORMAT_MASK_DTS)
{
  parser = 0;
  reset();
}

int
AudioDecoder::get_info(char *_buf, size_t _len) const
{
  return parser? parser->get_info(_buf, _len): 0;
}

void
AudioDecoder::reset()
{
  NullFilter::reset();
  sync_helper.reset();
  if (parser)
    parser->reset();

  stream_spk = spk_unknown;
  out_spk = spk_unknown;
}

bool
AudioDecoder::is_ofdd() const
{
  return true;
}


bool
AudioDecoder::set_input(Speakers _spk)
{
  if (!NullFilter::set_input(_spk))
    return false;

  switch (_spk.format)
  {
    case FORMAT_MPA: parser = &mpa; break;
    case FORMAT_AC3: parser = &ac3; break;
    case FORMAT_DTS: parser = &dts; break;
    default:
      return false;
  }
  parser->reset();
  return true;
}

bool 
AudioDecoder::process(const Chunk *_chunk)
{
  if (!parser)
    return false;

  // we must ignore dummy chunks
  if (_chunk->is_dummy())
    return true;

  FILTER_SAFE(receive_chunk(_chunk));

  sync_helper.receive_sync(sync, time);
  sync = false;

  load_frame();

  // if we did not start a stream we must
  // forget about current stream on flushing
  // and drop data currently buffered
  // (flushing state is also dropped so we 
  // do not pass eos event in this case)
  if (flushing && out_spk == spk_unknown)
    reset();

  return true;
}

Speakers
AudioDecoder::get_output() const
{
  return out_spk;
}

bool
AudioDecoder::is_empty() const
{
  if (parser)
    return !parser->is_frame_loaded() && !flushing;
  else
    return !flushing;
}

bool 
AudioDecoder::get_chunk(Chunk *_chunk)
{
  if (!parser) 
    return false;

  if (!parser->is_frame_loaded())
  {
    // flushing (see is_empty())
    _chunk->set_empty(out_spk, 0, 0, flushing);
    flushing = false;
    return true;
  }
  else if (!parser->decode_frame())
  {
    // decoding error
    // send dummy chunk and load next frame
    _chunk->set_dummy();
    load_frame();
    return true;
  }
  else if (parser->get_spk() != stream_spk)
  {
    // output format change flushing
    _chunk->set_empty(out_spk, 0, 0, true);
    stream_spk = parser->get_spk();
    out_spk = stream_spk;
    out_spk.format = FORMAT_LINEAR;
    return true;
  }
  else
  {
    // fill output chunk
    _chunk->set_linear(out_spk, parser->get_samples(), parser->get_nsamples());
    // timimg
    sync_helper.send_sync(_chunk);
    load_frame();
    return true;
  }

  // never be here
  return true;
}

bool
AudioDecoder::load_frame()
{
  uint8_t *buf_ptr = rawdata;
  uint8_t *end_ptr = rawdata + size;

  if (parser->load_frame(&buf_ptr, end_ptr))
  {
    if (stream_spk == spk_unknown)
    {
      stream_spk = parser->get_spk();
      out_spk = stream_spk;
      out_spk.format = FORMAT_LINEAR;
    }
    sync_helper.set_syncing(true);
    drop_rawdata(buf_ptr - rawdata);
    return true;
  }
  else
  {
    // we must not drop sync_helper's syncing state 
    // if it was no data to loaded
    if (size) sync_helper.set_syncing(false);
    drop_rawdata(buf_ptr - rawdata);
    return false;
  }
}
