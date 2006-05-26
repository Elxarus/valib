#ifndef FILTER_GRAPH_H
#define FILTER_GRAPH_H

#include "filter.h"
#include "stdio.h" // snprint

static const int graph_nodes = 32;
 
static const int node_err = -1;
static const int node_end = graph_nodes;

class FilterGraph : public Filter
{
protected:
  /////////////////////////////////////////////////////////
  // Two-way filter chain structure
  //
  // Last element is a special end-node
  // node_end index points to the end-node
  // next[node_end] points to the first node
  // prev[node_end] points to the last node
  // filter[node_end] does not exist

  int next[graph_nodes + 1];
  int prev[graph_nodes + 1];
  Filter *filter[graph_nodes];

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

  /////////////////////////////////////////////////////////
  // Drop filter chain and put graph into initial state
  //
  // void drop_chain()
  // updates filter lists (forward and reverse)
  // updates input/output formats and ofdd status

  void drop_chain()
  {
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
    // if ofdd filter is in transition state then drop 
    // the rest of the chain and set output format to 
    // spk_unknown

    if (spk == spk_unknown)
    {
      ofdd = true;

      next[node] = node_end;
      prev[node_end] = node;
      return true;
    }

    ///////////////////////////////////////////////////////
    // find the next filter

    int next_node = get_next(node, spk);

    // runtime protection
    // we may check get_next() result only here because
    // in all other cases wrong get_next() result forces
    // chain to rebuild and we'll get here anyway

    if (next_node == node_err || next_node != node_end)
      if (next_node < 0 || next_node >= graph_nodes)
        return false;
  
    ///////////////////////////////////////////////////////
    // end of the filter chain
    // drop the rest of the chain and 
    // set output format

    if (next_node == node_end)
    {
      next[node] = node_end;
      prev[node_end] = node;
      return true;
    }

    ///////////////////////////////////////////////////////
    // build new filter into the filter chain

    // create filter
    filter[next_node] = init_filter(next_node, spk);

    // runtime protection
    // must do it BEFORE updating of filter lists
    // otherwise filter list may be broken

    if (!filter[next_node])
      return false;

    // init filter
    // must do it BEFORE updating of filter lists
    // otherwise filter list may be broken

    FILTER_SAFE(filter[next_node]->set_input(spk));

    // update filter lists
    next[node] = next_node;
    prev[next_node] = node;
    next[next_node] = node_end;
    prev[node_end] = next_node;

    // update ofdd status
    // aggregate is data-dependent if chain has
    // at least one ofdd filter
    ofdd = false;
    node = next[node_end];
    while (node != node_end)
    {
      ofdd |= filter[node]->is_ofdd();
      node = next[node];
    }

    return true;
  }

  /////////////////////////////////////////////////////////
  // Chain data flow

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

protected:
  bool ofdd;

public:
  FilterGraph()
  {
    drop_chain();
  };

  virtual ~FilterGraph()
  {};

  /////////////////////////////////////////////////////////
  // Overridable functions

  virtual const char *get_name(int node) const = 0;
  virtual const Filter *get_filter(int node) const = 0;
  virtual Filter *init_filter(int node, Speakers spk) = 0;

  virtual int get_next(int node, Speakers spk) const = 0;

  /////////////////////////////////////////////////////////
  // Print chain
  //
  // int chain_text(char *buf, size_t buf_size)
  // buf - output buffer
  // buf_size - output buffer size
  // returns number of printed bytes

  int chain_text(char *buf, size_t buf_size)
  {
    size_t i;
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

  /////////////////////////////////////////////////////////
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
    // return dummy chunk

    chunk->set_dummy();

    return true;
  };
};


class FilterChain : public FilterGraph
{
protected:
  Filter *chain[graph_nodes];
  char   *desc[graph_nodes];
  int     chain_size;

public:
  FilterChain()
  {
    chain_size = 0;
  };

  ~FilterChain()
  {
    drop();
  };

  /////////////////////////////////////////////////////////
  // FilterChain interface

  bool add_front(Filter *_filter, const char *_desc)
  {
    if (chain_size >= graph_nodes)
      return false;

    for (int i = chain_size; i > 0; i--)
    {
      chain[i] = chain[i-1];
      desc[i] = desc[i-1];
    }

    chain[0] = _filter;
    desc[0] = strdup(_desc);
    chain_size++;
    return true;
  }

  bool add_back(Filter *_filter, const char *_desc)
  {
    if (chain_size >= graph_nodes)
      return false;

    chain[chain_size] = _filter;
    desc[chain_size] = strdup(_desc);
    chain_size++;
    return true;
  }

  void drop()
  {
    drop_chain();
    for (int i = 0; i < chain_size; i++)
      if (desc[i]) delete desc[i];
    chain_size = 0;
  }

  /////////////////////////////////////////////////////////
  // FilterGraph overrides

  const char *get_name(int _node) const
  {
    if (_node >= chain_size)
      return 0;

    return desc[_node];
  }

  const Filter *get_filter(int _node) const
  {
    if (_node >= chain_size)
      return 0;

    return chain[_node];
  }

  Filter *init_filter(int _node, Speakers _spk)
  {
    if (_node >= chain_size)
      return 0;

    return chain[_node];
  }

  int get_next(int _node, Speakers _spk) const
  {
    if (_node == node_end)
      return chain_size? 0: node_end;

    if (_node >= chain_size - 1)
      return node_end;

    return _node + 1;
  }
};


#endif
