#ifndef LINEAR_FILTER_H
#define LINEAR_FILTER_H

#include "../filter.h"
#include "../sync.h"

///////////////////////////////////////////////////////////////////////////////
// Simplified filter interface for linear processing

class LinearFilter : public Filter
{
private:
  SyncHelper sync_helper;
  bool       flushing;

  samples_t  samples;
  size_t     size;
  samples_t  out_samples;
  size_t     out_size;
  int        buffered_samples;

protected:
  Speakers   in_spk;
  Speakers   out_spk;

public:
  LinearFilter();
  virtual ~LinearFilter();

  /////////////////////////////////////////////////////////////////////////////
  // Interface to override

  virtual bool query(Speakers spk) const;
  virtual bool init(Speakers spk, Speakers &out_spk);
  virtual void reset_state();

  virtual bool process_samples(samples_t in, size_t in_size, samples_t &out, size_t &out_size, size_t &gone);
  virtual bool process_inplace(samples_t in, size_t in_size);
  virtual bool flush(samples_t &out, size_t &out_size);

  virtual bool need_flushing() const;
  virtual bool want_reinit() const;

  /////////////////////////////////////////////////////////////////////////////
  // Filter interface (fully implemented)

  virtual void reset();
  virtual bool is_ofdd() const;

  virtual bool query_input(Speakers spk) const;
  virtual bool set_input(Speakers spk);
  virtual Speakers get_input() const;
  virtual bool process(const Chunk *chunk);

  virtual Speakers get_output() const;
  virtual bool is_empty() const;
  virtual bool get_chunk(Chunk *chunk);
};

#endif
