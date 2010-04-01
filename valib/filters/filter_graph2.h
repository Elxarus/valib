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

#include <list>
#include <string>

#include "../filter2.h"
#include "passthrough.h"

class FilterGraph2 : public SimpleFilter
{
private:
  enum state_t { state_init, state_empty, state_processing, state_rebuild, state_done_flushing };

  struct Node
  {
    int id;
    Node *next;
    Node *prev;

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

  bool is_new_stream;

  void truncate(Node *node);
  bool build_chain(Node *node);

  bool chain_is_empty() const;
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

  virtual std::string get_name(int id) const
  { return 0; }

public:
  static const int node_start = -1;
  static const int node_end   = -2;
  static const int node_err   = -3;

  FilterGraph2();
  virtual ~FilterGraph2();

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  virtual bool init(Speakers spk);
  virtual void uninit();

  virtual bool process(Chunk2 &in, Chunk2 &out);
  virtual bool flush(Chunk2 &out);
  virtual void reset();

  virtual bool can_open(Speakers new_spk) const
  { return next_id(node_start, new_spk) != node_err; }

  virtual bool new_stream() const
  { return is_new_stream; }

  virtual Speakers get_input() const
  { return start.filter->get_input(); }

  virtual Speakers get_output() const
  { return end.filter->get_output(); }
};

///////////////////////////////////////////////////////////////////////////////
// FilterChain
// Connects filters one after another
///////////////////////////////////////////////////////////////////////////////

class FilterChain2 : public FilterGraph2
{
protected:
  struct Node
  {
    int id;
    Filter2 *filter;
    std::string name;

    Node(int id_, Filter2 *filter_, const std::string &name_ = std::string()):
    id(id_), filter(filter_), name(name_) {}

    bool operator ==(int id_)          const { return id == id_;         }
    bool operator ==(Filter2 *filter_) const { return filter == filter_; }
  };

  int id;
  std::list<Node> nodes;
  typedef std::list<Node>::const_iterator const_list_iter;

  /////////////////////////////////////////////////////////
  // FilterGraph overrides

  virtual int next_id(int id, Speakers spk) const;
  virtual Filter2 *init_filter(int id, Speakers spk);
  virtual std::string get_name(int id) const;

public:
  FilterChain2();
  virtual ~FilterChain2();

  /////////////////////////////////////////////////////////
  // FilterChain interface
  // add_front()
  //   Inserts a filter at the beginning of the chain.
  //
  //   You can add a filter during processing, regular
  //   chain rebuild is initiated.
  //
  // add_back()
  //   Add a filter to the end of the chain.
  //
  //   You can add a filter during processing, regular
  //   chain rebuild is initiated.
  //
  // remove()
  //   Remove a filter from the chain.
  //
  //   You can remove a filter during processing, regular
  //   chain rebuild is initiated.
  //
  //   Note, that filter is used until the end of the
  //   rebuild process. Use destroy() if you need to
  //   release the filter immediately.
  //
  // clear()
  //   Remove all filters form the chain. You can clear
  //   during processing, regular chain rebuild is initiated.
  //
  //   Note, that filters are used until the end of the
  //   rebuild process. Use destroy() if you need to
  //   release filters immediately.
  //
  // destroy()
  //   Destroy the chain and release all filters
  //   immediately. Current processing is interrupted.

  bool add_front(Filter2 *filter, const std::string &name = std::string());
  bool add_back(Filter2 *filter, const std::string &name = std::string());

  void remove(Filter2 *filter);
  void clear();

  void destroy();
};

#endif
