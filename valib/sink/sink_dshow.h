/*
  DirectShow output sink
*/

#ifndef VALIB_SINK_DSHOW_H
#define VALIB_SINK_DSHOW_H

#include <windows.h>
#include <streams.h>
#include "../sink.h"

class DShowSink : public CTransformOutputPin, public SimpleSink
{
protected:
  bool send_mt;             // send media type with next sample
  bool send_dc;             // send discontinuity with next sample
  bool preroll;             // mark output samples as preroll
  HRESULT hr;               // error code from last process() call

  bool query_downstream(const CMediaType *mt) const;
  bool set_downstream(const CMediaType *mt);

  HRESULT CheckMediaType(const CMediaType *mt);
  HRESULT SetMediaType(const CMediaType *mt);

public:
  //! Processing error exception
  struct Error : public Sink::Error {};

  DShowSink(CTransformFilter* pFilter, HRESULT* phr);

  void send_discontinuity()          { send_dc = true; }
  void send_mediatype()              { send_mt = true; }
  void set_preroll()                 { preroll = true; }
  void unset_preroll()               { preroll = false; }
  HRESULT get_hresult()              { return hr;      }

  /////////////////////////////////////////////////////////
  // Sink interface

  virtual bool can_open(Speakers spk) const;
  virtual bool init();
  virtual void process(const Chunk &chunk);
};

#endif
