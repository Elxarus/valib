#include "filter_graph2.h"

const int FilterGraph2::node_start = -1;
const int FilterGraph2::node_end = -2;
const int FilterGraph2::node_err = -3;

FilterGraph2::FilterGraph2()
{
  start.id = node_start;
  start.next = &end;
  start.prev = &end;
  start.filter = &pass_start;
  start.state = ns_dirty;

  end.id = node_end;
  end.next = &start;
  end.prev = &start;
  end.filter = &pass_end;
  end.state = ns_ok;
}

FilterGraph2::~FilterGraph2()
{
  truncate(&start);
}

///////////////////////////////////////////////////////////
// SimpleFilter overrides

bool
FilterGraph2::init(Speakers spk)
{
  if (next_id(node_start, spk) == node_err)
    return false;

  destroy();
  start.filter->open(spk);
  return build_chain(&start);
}

void
FilterGraph2::uninit()
{
  destroy();
}

bool
FilterGraph2::process(Chunk2 &in, Chunk2 &out)
{
  if (!end.output.is_dummy())
  {
    out = end.output;
    end.output.set_empty();
    return true;
  }

  if (start.output.is_dummy())
  {
    start.output = in;
    in.set_empty();
  }

  Node *node = end.prev;
  while (node->id != node_end)
  {
    // Find a full node
    if (node->output.is_dummy())
    {
      node = node->prev;
      continue;
    }

    // Process data downstream
    node->next->filter->process(node->output, node->next->output);
    node = node->next;
  }

  out = end.output;
  end.output.set_empty();
  return true;
}

bool
FilterGraph2::flush(Chunk2 &out)
{
  return true;
}

void
FilterGraph2::reset()
{
  Node *node = start.next;
  while (node->id != node_end)
  {
    node->filter->reset();
    node->output.set_empty();
    node = node->next;
  }
}


///////////////////////////////////////////////////////////
// Truncate the chain
// truncate(node)  - truncate everything after the node
// truncate(start) - delete the whole chain

void
FilterGraph2::truncate(Node *node)
{
  node = node->next;
  while (node->id != node_end)
  {
    node->filter->close();
    uninit_filter(node->id);
    node->prev->next = node->next;
    node->next->prev = node->prev;
    Node *next_node = node->next;
    delete node;
    node = next_node;
  }
}

///////////////////////////////////////////////////////////
// Extend the chain
// Truncate the chain if nessesary and insert a correct
// node after the node specified.

bool
FilterGraph2::extend(Node *node)
{
  const Speakers next_spk = node->filter->get_output();

  /////////////////////////////////////////////////////////
  // if ofdd filter is in transition state then set output
  // format to spk_unknown

  if (next_spk == spk_unknown)
  {
    // ofdd = true;
    return true;
  }

  /////////////////////////////////////////////////////////
  // find the next node

  int node_id = node->id;
  int next_node_id = next_id(node_id, next_spk);

  if (next_node_id == node_err)
    return false;

  // end of the filter chain
  // truncate the chain

  if (next_node_id == node_end)
  {
    if (node->next->id != node_end)
      truncate(node);

    end.state = ns_ok;
    end.output.set_empty();
    return end.filter->open(next_spk);
  }

  // no need to build a new node
  // just reopen the next node

  if (next_node_id == node->next->id)
  {
    node->next->state = ns_ok;
    node->next->output.set_empty();
    return node->next->filter->open(next_spk);
  }

  /////////////////////////////////////////////////////////
  // Build a new node

  // create & init the filter

  Filter2 *filter = init_filter(next_node_id, next_spk);
  if (!filter || !filter->open(next_spk))
    return false;

  // update filter lists

  Node *next_node   = new Node;
  next_node->id     = next_node_id;
  next_node->next   = &end;
  next_node->prev   = node;
  next_node->name   = get_name(next_node_id);
  next_node->filter = filter;
  next_node->state  = ns_ok;
  node->next = next_node;

  return true;
}

///////////////////////////////////////////////////////////
// Build the whole chain starting from the node given

bool
FilterGraph2::build_chain(Node *node)
{
  while (node->id != node_end)
  {
    if (!extend(node))
      return false;
    node = node->next;
  };
  return true;
}

void
FilterGraph2::invalidate()
{}

void
FilterGraph2::destroy()
{
  truncate(&start);
}

///////////////////////////////////////////////////////////////////////////////
// FilterChain
///////////////////////////////////////////////////////////////////////////////

FilterChain2::FilterChain2()
{
  chain_size = 0;
}

FilterChain2::~FilterChain2()
{
  for (int i = 0; i < chain_size; i++)
    safe_delete(desc[i]);
}

bool
FilterChain2::add_front(Filter2 *filter_, const char *desc_)
{
  if (chain_size >= array_size(chain))
    return false;

  for (int i = chain_size; i > 0; i--)
  {
    chain[i] = chain[i-1];
    desc[i] = desc[i-1];
  }

  chain[0] = filter_;
  desc[0] = strdup(desc_);
  chain_size++;
  return true;
}

bool
FilterChain2::add_back(Filter2 *filter_, const char *desc_)
{
  if (chain_size >= array_size(chain))
    return false;

  chain[chain_size] = filter_;
  desc[chain_size] = strdup(desc_);
  chain_size++;
  return true;
}

void
FilterChain2::drop()
{
  destroy();
  for (int i = 0; i < chain_size; i++)
    safe_delete(desc[i]);
  chain_size = 0;
}

int
FilterChain2::next_id(int id, Speakers spk) const
{
  if (id == node_start)
    return chain_size? 0: node_end;

  if (id < 0)
    return node_err;

  if (id >= chain_size - 1)
    return node_end;

  if (chain[id+1]->can_open(spk))
    return id + 1;
  else
    return node_err;
}

Filter2 *
FilterChain2::init_filter(int id, Speakers spk)
{
  if (id < 0 || id >= chain_size)
    return 0;

  return chain[id];
}

const char *
FilterChain2::get_name(int id) const
{
  if (id < 0 || id >= chain_size)
    return 0;

  return desc[id];
}
