/*
  FilterGraph
  Base class for aggregate filters with custom data flow. Implements logic for
  dynamic data flow change.

  Aggregate filter contains a set of filters, where output of one filter goes
  to the input of another (filter chain). The chain may be dynamically changed:
  due to:

  * Format change in between of two filters in the chain
  * Parameter change

  When format change in the chain change occurs, downstream filters are flushed,
  so it will be no gaps in the output stream. It is important because filters
  may buffer significant amount of data (several seconds for instance).
*/

#ifndef VALIB_FILTER_GRAPH2_H
#define VALIB_FILTER_GRAPH2_H

#include "../filter2.h"
#include "passthrough.h"

class FilterGraph2 : public SimpleFilter
{
private:
  enum state_t { state_empty, state_processing, state_rebuild, state_done_flushing };

  struct Node
  {
    int id;
    Node *next;
    Node *prev;
    const char *name;

    Filter2 *filter;
    Chunk2   input;
    Chunk2   output;

    state_t  state;
    bool     flushing;
  };

  Node start;
  Node end;

  Passthrough pass_start;
  Passthrough pass_end;

  void truncate(Node *node);
  bool build_chain(Node *node);
  bool process_chain(Chunk2 &out);

protected:
  /////////////////////////////////////////////////////////
  // invalidate()
  //   Rebuild the chain if nessesary. Call this when
  //   filter chain is suspected to change. For example
  //   after the change of a parameter that switches some
  //   filter on or off. In this case filter chain will be
  //   gracefully rebuilt without interruption of the data
  //   flow.
  //
  //   Note, that filter to be removed will present in the
  //   graph after invalidate() call. Deallocation of the
  //   filter will have place only after correct flushing
  //   of this filter. So it is not suitable when you need
  //   to immediately remove some filter.
  //
  // destroy()
  //   Immediately free all filters and destroy the chain.
  //   It is not a graceful way to remove all filters from
  //   the chain duriong the data flow because all filters
  //   are removed with all the data buffered.

  void invalidate();
  void destroy();

  /////////////////////////////////////////////////////////
  // Interface to override
  //
  // int next_id(int id, Speakers spk)
  //   Return id of the next node in the chain
  //
  // Filter2 *init_filter(int id, Speakers spk)
  //   Prepare the filter to be included into the chain.
  //
  // void uninit_filter(int id)
  //   Filter was removed from the chain.
  //
  // const char *get_name()
  //   Return the filter's name.

  virtual int next_id(int id, Speakers spk) const
  { return id == node_start? node_end: node_err; }

  virtual Filter2 *init_filter(int id, Speakers spk)
  { return 0; }

  virtual void uninit_filter(int id)
  {}

  virtual const char *get_name(int id) const
  { return 0; }

public:
  static const int node_start;
  static const int node_end;
  static const int node_err;

  FilterGraph2();
  virtual ~FilterGraph2();

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  virtual bool init(Speakers spk);
  virtual void uninit();

  virtual bool process(Chunk2 &in, Chunk2 &out);
  virtual bool flush(Chunk2 &out);
  virtual void reset();

  virtual bool can_open(Speakers spk) const
  { return next_id(node_start, spk) != node_err; }

  virtual Speakers get_input() const
  { return start.filter->get_input(); }

  virtual Speakers get_output() const
  { return end.filter->get_output(); }
};

class FilterChain2 : public FilterGraph2
{
protected:
  Filter2 *chain[32];
  char    *desc[32];
  int      chain_size;

  /////////////////////////////////////////////////////////
  // FilterGraph overrides

  virtual int next_id(int id, Speakers spk) const;
  virtual Filter2 *init_filter(int id, Speakers spk);
  virtual const char *get_name(int id) const;

public:
  FilterChain2();
  virtual ~FilterChain2();

  /////////////////////////////////////////////////////////
  // FilterChain interface

  bool add_front(Filter2 *filter, const char *desc);
  bool add_back(Filter2 *filter, const char *desc);
  void drop();
};

#endif
