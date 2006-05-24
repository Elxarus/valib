#ifndef FILTER_GRAPH_H
#define FILTER_GRAPH_H

#include "filter.h"
#include "stdio.h" // snprint

static const int graph_nodes = 32;
 
static const int node_end = 0; 
static const int node_err = -1;

class FilterGraph : public Filter
{
protected:
  Speakers in_spk;
  Speakers out_spk;
  bool ofdd;

  int next[graph_nodes];
  int prev[graph_nodes];
  Filter *filter[graph_nodes];

public:
  virtual const char *get_name(int node) const = 0;
  virtual const Filter *get_filter(int node) const = 0;
  virtual Filter *init_filter(int node, Speakers spk) = 0;

  virtual int get_next(int node, Speakers spk) const = 0;

  bool process_internal()
  {
    int next_node;
    Speakers spk;
    Chunk chunk;

    int node = prev[node_end];
    while (node != node_end)
    {
      /////////////////////////////////////////////////////
      // find full filter

      if (filter[node]->is_empty())
      {
        // we need more data from upstream.
        node = prev[node];
        continue;
      }

      /////////////////////////////////////////////////////
      // filter is full so get_output() must always
      // report format of next output chunk and
      // therefore we can always find next node

      spk = filter[node]->get_output();
      next_node = get_next(node, spk);

      /////////////////////////////////////////////////////
      // rebuild the filter chain if changed

      if (next_node != node[next])
      {
        FILTER_SAFE(build_chain(node));
        next_node = next[node];
      }

      /////////////////////////////////////////////////////
      // process data downstream

      if (next_node != node_end)
      {
        FILTER_SAFE(filter[node]->get_chunk(&chunk));
        FILTER_SAFE(filter[next_node]->process(&chunk));
      }

      node = next_node;
    }

    return true;
  }

  /////////////////////////////////////////////////////////
  // Drop filter chain and put graph into initial state
  //
  // void drop_chain()
  // updates filter lists (forward and reverse)
  // updates input/output formats and ofdd status

  void drop_chain()
  {
    in_spk = spk_unknown;
    out_spk = spk_unknown;
    ofdd = false;

    next[node_end] = node_end;
    prev[node_end] = node_end;
  }

  /////////////////////////////////////////////////////////
  // Add new node into the chain
  // 
  // bool add_node(int node, Speakers spk)
  // node = parent node
  // spk = input format for a new node
  //
  // updates filter lists (forward and reverse)
  // updates output format and ofdd status

  bool add_node(int node, Speakers spk)
  {
    ///////////////////////////////////////////////////////
    // ofdd filter in transition state
    // drop the rest of the chain and 
    // set output format to spk_unknown

    if (spk == spk_unknown)
    {
      out_spk = spk_unknown;
      ofdd = true;

      next[node] = node_end;
      prev[node_end] = node;
      return true;
    }

    ///////////////////////////////////////////////////////
    // find the next filter

    int next_node = get_next(node, spk);

    // runtime protection
    if ((next_node == node_err) || (next_node >= graph_nodes) || (next_node < 0))
      return false;
  
    ///////////////////////////////////////////////////////
    // end of the filter chain
    // drop the rest of the chain and 
    // set output format

    if (next_node == node_end)
    {
      out_spk = spk;
      next[node] = node_end;
      prev[node_end] = node;
      return true;
    }

    ///////////////////////////////////////////////////////
    // build new filter into the filter chain

    // create filter
    filter[next_node] = init_filter(next_node, spk);

    // runtime protection
    if (!filter[next_node])
      return false;

    // init filter
    // (must do it BEFORE updating of filter lists)
    FILTER_SAFE(filter[next_node]->set_input(spk));

    // update ofdd status
    if (filter[next_node]->is_ofdd())
      ofdd = true;

    // update filter lists
    next[node] = next_node;
    prev[next_node] = node;
    next[next_node] = node_end;
    prev[node_end] = next_node;

    return true;
  }

  /////////////////////////////////////////////////////////
  // Build filter chain after the specified node
  //
  // bool build_chain(int node)
  // node - existing node after which we must build 
  //   the chain
  // updates nothing (uses add_node() to update the chain)

  bool build_chain(int node)
  {
    Speakers spk;
    while (node != node_end)
    {
      spk = filter[node]->get_output();
      FILTER_SAFE(add_node(node, spk));
      node = next[node];
    }
    return true;
  }

  int chain_text(char *buf, size_t buf_size)
  {
    int i;
    Speakers spk;

    char *buf_ptr = buf;
    int node = next[node_end];

    if (node != node_end)
    {
      spk = filter[node]->get_input();

      if (spk.mask || spk.sample_rate)
        i = _snprintf(buf_ptr, buf_size, "(%s %s %i)", spk.format_text(), spk.mode_text(), spk.sample_rate);
      else
        i = _snprintf(buf_ptr, buf_size, "(%s)", spk.format_text());

      buf_ptr += i;
      buf_size = (buf_size > i)? buf_size - i: 0;
    }

    while (node != node_end)
    {
      spk = filter[node]->get_output();

      if (spk.mask || spk.sample_rate)
        i = _snprintf(buf_ptr, buf_size, " -> %s -> (%s %s %i)", get_name(node), spk.format_text(), spk.mode_text(), spk.sample_rate);
      else
        i = _snprintf(buf_ptr, buf_size, " -> %s -> (%s)", get_name(node), spk.format_text());
      buf_ptr += i;
      buf_size = (buf_size > i)? buf_size - i: 0;

      node = next[node];
    }
    return buf_ptr - buf;
  }

  // Filter interface
  virtual void reset()
  {
    int node = next[node_end];
    while (node != node_end)
    {
      filter[node]->reset();
      if (filter[node]->is_ofdd())
      {
        // cut the chain at ofdd filter
        build_chain(node);
        return;
      }
      node = next[node];
    }
  }

  virtual bool is_ofdd() const
  {
    return ofdd;
  }

  virtual bool query_input(Speakers spk) const
  {
    int node = get_next(node_end, spk);
    const Filter *f = get_filter(node);
    return f? f->query_input(spk): false;
  }

  virtual bool set_input(Speakers spk)
  {
    reset();
    if (!query_input(spk))
      return false;

    drop_chain();
    FILTER_SAFE(add_node(node_end, spk));
    FILTER_SAFE(build_chain(next[node_end]));
    in_spk = spk;
    return true;
  }

  virtual Speakers get_input() const
  {
    if (next[node_end] == node_end)
      return spk_unknown;
    else      
      return filter[next[node_end]]->get_input();
  };

  virtual bool process(const Chunk *chunk)
  {
    if (chunk->is_dummy())
      return true;

    int node = get_next(node_end, chunk->spk);

    if (node != next[node_end])
    {
      if (node == node_err)
        return false;

      FILTER_SAFE(add_node(node_end, chunk->spk));
      FILTER_SAFE(build_chain(node));
    }

    FILTER_SAFE(filter[next[node_end]]->process(chunk));
    FILTER_SAFE(process_internal());
    return true;
  };

  virtual Speakers get_output() const
  {
    if (prev[node_end] == node_end)
      return spk_unknown;
    else
      return filter[prev[node_end]]->get_output();
  };

  virtual bool is_empty() const
  {
    ///////////////////////////////////////////////////////
    // graph is not empty if there're at least one 
    // non-empty filter

    int node = prev[node_end];
    while (node != node_end)
    {
      if (!filter[node]->is_empty())
        return false;

      node = prev[node];
    }
    return true;
  };

  virtual bool get_chunk(Chunk *chunk)
  {
    Filter *f = filter[prev[node_end]];

    ///////////////////////////////////////////////////////
    // out_spk may change in process_internal
    // therefore we must prepare empty chunk now

    chunk->set_empty(out_spk);

    ///////////////////////////////////////////////////////
    // if there're something to output from the last chunk
    // get it...

    if (!f->is_empty())
      return f->get_chunk(chunk);

    ///////////////////////////////////////////////////////
    // if the last chunk is empty process data internally
    // and try to get output afterwards

    FILTER_SAFE(process_internal());
    if (!f->is_empty())
      return f->get_chunk(chunk);

    ///////////////////////////////////////////////////////
    // return empty chunk (prepared before)

    return true;
  };
};

#endif
