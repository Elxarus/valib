/*
  DirectShow output sink
*/

#ifndef DSHOW_SINK_H
#define DSHOW_SINK_H

#include <streams.h>
#include "filter.h"

bool mt2spk(CMediaType mt, Speakers &spk);
bool spk2mt(Speakers spk, CMediaType &mt, bool use_wfx);

class DShowSink : public CTransformOutputPin, public Sink
{
protected:
  Speakers spk;          // output configuration
  bool send_mt;          // send media type with next sample
  bool discontinuity;    // send discontinuity with next sample

  bool query_downstream(const CMediaType *mt) const;
  bool set_downstream(const CMediaType *mt);

public:
  DShowSink(CTransformFilter* pFilter, HRESULT* phr);

  void send_discontinuity()          { discontinuity = true; }
  void send_mediatype()              { send_mt = true;       }

  // Sink interface
  virtual bool query_input(Speakers spk) const;
  virtual bool set_input(Speakers spk);
  virtual Speakers get_input() const;
  virtual bool process(const Chunk *chunk);
};

#endif
