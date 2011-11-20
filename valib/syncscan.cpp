#include <iostream>
#include "syncscan.h"

using namespace std;

const SyncTrie SyncTrie::any(1);
const SyncTrie SyncTrie::one(1, 1);
const SyncTrie SyncTrie::zero(0, 1);

///////////////////////////////////////////////////////////////////////////////
// SyncTrie
///////////////////////////////////////////////////////////////////////////////

SyncTrie::SyncTrie(uint32_t value, unsigned bits): depth(bits)
{
  if (bits == 0)
    return;

  graph.resize(bits);
  value <<= 32 - bits;
  for (unsigned bit = 0; bit < bits; bit++)
  {
    graph[bit] = Node(node_deny, node_deny);
    graph[bit].children[value >> 31] = bit+1;
    value <<= 1;
  }

  if (graph[bits-1].left() != node_deny)
    graph[bits-1].left() = node_allow;

  if (graph[bits-1].right() != node_deny)
    graph[bits-1].right() = node_allow;
}

SyncTrie::SyncTrie(unsigned bits): depth(bits)
{
  if (bits == 0)
    return;

  graph.resize(bits);
  for (unsigned bit = 0; bit < bits; bit++)
    graph[bit] = Node(bit+1, bit+1);
  graph[bits-1] = Node(node_allow, node_allow);
}

SyncTrie::SyncTrie(const std::string &trie): depth(0)
{
  deserialize(trie);
}

int
SyncTrie::insert_node(Graph &gr)
{
  gr.push_back(Node());
  return gr.size() - 1;
}

int
SyncTrie::copy(Graph &result, const Graph &gr, int node)
{
  int l = gr[node].left();
  int r = gr[node].right();
  int new_l = l, new_r = r;

  int new_node = insert_node(result);

  if (l != node_allow && l != node_deny)
    new_l = copy(result, gr, l);

  if (r != node_allow && r != node_deny && r == l)
    new_r = new_l;
  else if (r != node_allow && r != node_deny)
    new_r = copy(result, gr, r);

  result[new_node] = Node(new_l, new_r);
  return new_node;
}

size_t
SyncTrie::find_depth(Graph &gr, int node)
{
  size_t left_depth = 0;
  size_t right_depth = 0;
  int l = gr[node].left();
  int r = gr[node].right();

  if (l != node_allow && l != node_deny)
    left_depth = find_depth(gr, l);
  if (r != node_allow && r != node_deny && r != l)
    right_depth = find_depth(gr, r);

  return 1 + MAX(left_depth, right_depth);
}

size_t
SyncTrie::find_depth(Graph &gr)
{
  return gr.size()? find_depth(gr, 0): 0;
}

int
SyncTrie::merge(Graph &result, const Graph &gr1, int node1, const Graph &gr2, int node2)
{
  int l1 = gr1[node1].left();
  int r1 = gr1[node1].right();
  int l2 = gr2[node2].left();
  int r2 = gr2[node2].right();
  int l, r;

  // Create a new node
  int new_node = insert_node(result);

  // Merge left subtree
  if (l1 == node_allow || l2 == node_allow)
    l = node_allow;
  else if (l1 == node_deny && l2 == node_deny)
    l = node_deny;
  else if (l1 != node_deny && l2 != node_deny)
    l = merge(result, gr1, l1, gr2, l2);
  else if (l1 != node_deny)
    l = copy(result, gr1, l1);
  else // (l2 != node_deny)
    l = copy(result, gr2, l2);

  if (l1 == r1 && l2 == r2)
    // Both branches are equal
    r = l;
  else
  {
    // Merge right subtree
    if (r1 == node_allow || r2 == node_allow)
      r = node_allow;
    else if (r1 == node_deny && r2 == node_deny)
      r = node_deny;
    else if (r1 != node_deny && r2 != node_deny)
      r = merge(result, gr1, r1, gr2, r2);
    else if (r1 != node_deny)
      r = copy(result, gr1, r1);
    else // (r2 != node_deny)
      r = copy(result, gr2, r2);
  }

  result[new_node] = Node(l, r);
  return new_node;
}

int
SyncTrie::optimize(Graph &result, const Graph &gr, int node)
{
  int l = gr[node].left();
  int r = gr[node].right();

  int new_node = insert_node(result);
  if (l == r && l != node_allow && l != node_deny)
    l = r = optimize(result, gr, l);
  else
  {
    if (l != node_allow && l != node_deny)
      l = optimize(result, gr, l);
    if (r != node_allow && r != node_deny)
      r = optimize(result, gr, r);
  }

  // Do not create a new node for always allow & always deny node
  if ((l == node_allow && r == node_allow) || 
      (l == node_deny  && r == node_deny))
  {
    result.pop_back();
    return l;
  }

  result[new_node] = Node(l, r);
  return new_node;
}

void
SyncTrie::clear()
{
  graph.clear();
  depth = 0;
}

void
SyncTrie::invert()
{
  for (size_t node = 0; node < graph.size(); node++)
  {
    if (graph[node].left() == node_allow)
      graph[node].left() = node_deny;
    else if (graph[node].left() == node_deny)
      graph[node].left() = node_allow;

    if (graph[node].right() == node_allow)
      graph[node].right() = node_deny;
    else if (graph[node].right() == node_deny)
      graph[node].right() = node_allow;
  }
}

void
SyncTrie::merge(const SyncTrie &other)
{
  if (other.graph.size() == 0)
    return;

  if (graph.size() == 0)
  {
    graph = other.graph;
    depth = other.depth;
  }
  else
  {
    Graph new_graph;
    merge(new_graph, graph, 0, other.graph, 0);
    size_t new_depth = find_depth(new_graph);

    graph.swap(new_graph);
    depth = new_depth;
  }
}

void
SyncTrie::append(const SyncTrie &other)
{
  if (other.graph.size() == 0)
    return;

  if (graph.size() == 0)
  {
    graph = other.graph;
    depth = other.depth;
    return;
  }

  size_t current_size = graph.size();
  for (size_t node = 0; node < current_size; node++)
  {
    int l = graph[node].left();
    int r = graph[node].right();
    if (l != r)
    {
      if (l == node_allow)
        l = copy(graph, other.graph, 0);
      if (r == node_allow)
        r = copy(graph, other.graph, 0);
    }
    else if (l == node_allow)
      r = l = copy(graph, other.graph, 0);
    graph[node] = Node(l, r);
  }

  depth += other.depth;
}

void
SyncTrie::optimize()
{
  if (graph.size() > 0)
  {
    Graph new_graph;
    int result = optimize(new_graph, graph, 0);

    // Add "always allow" as root node to build "always allow" graph.
    // Such trie has no sense, but nessesary to keep formal correctness.
    if (result == node_allow)
      new_graph.push_back(Node(node_allow, node_allow));

    size_t new_depth = find_depth(new_graph);

    graph.swap(new_graph);
    depth = new_depth;
  }
}

void
SyncTrie::serialize(std::string &result, int node)
{
  int l = graph[node].left();
  int r = graph[node].right();

  if (l == node_allow && r == node_allow)
    result.append("A"); // allow all
  else if (l == node_allow && r == node_deny)
    result.append("O"); // terminating zero
  else if (l == node_deny && r == node_allow)
    result.append("I"); // terminating one
  else if (l == node_deny && r == node_deny)
    result.append("D"); // deny all
  else if (l == node_allow && r != node_allow && r != node_deny)
    result.append("R"); // terminating zero or one (right branch)
  else if (l == node_deny && r != node_allow && r != node_deny)
    result.append("i"); // only one
  else if (r == node_allow && l != node_allow && l != node_deny)
    result.append("L"); // terminating one or zero (left branch)
  else if (r == node_deny && l != node_allow && l != node_deny)
    result.append("o"); // only zero
  else if (l == r)
    result.append("x"); // any value
  else
    result.append("*"); // one or zero

  if (l == r && l != node_allow && l != node_deny)
    serialize(result, l);
  else
  {
    if (l != node_allow && l != node_deny)
      serialize(result, l);
    if (r != node_allow && r != node_deny)
      serialize(result, r);
  }
}

int
SyncTrie::deserialize(const std::string &s, size_t &pos, size_t &depth)
{
  if (pos >= s.size())
    throw EUnexpectedEndOfData();

  // Get next character, skip whitespace
  char c;
  do {
    c = s[pos];
    pos++;
  } while (c == ' ');

  int new_node = insert_node(graph);

  int l, r;
  switch (c)
  {
  case 'A':
    l = node_allow;
    r = node_allow;
    break;
  case 'O':
    l = node_allow;
    r = node_deny;
    break;
  case 'I':
    l = node_deny;
    r = node_allow;
    break;
  case 'D':
    l = node_deny;
    r = node_deny;
    break;
  case 'R':
    l = node_allow;
    r = deserialize(s, pos, depth);
    break;
  case 'i':
    l = node_deny;
    r = deserialize(s, pos, depth);
    break;
  case 'L':
    l = deserialize(s, pos, depth);
    r = node_allow;
    break;
  case 'o':
    l = deserialize(s, pos, depth);
    r = node_deny;
    break;
  case 'x':
    l = deserialize(s, pos, depth);
    r = l;
    break;
  case '*':
  {
    size_t depth1 = 0, depth2 = 0;
    l = deserialize(s, pos, depth1);
    r = deserialize(s, pos, depth2);
    depth += MAX(depth1, depth2);
    break;
  }
  default:
    throw EUnexpectedSymbol();
  }
  depth++;
  graph[new_node] = Node(l, r);
  return new_node;
}

///////////////////////////////////////////////////////////////////////////////
// SyncScan
// Booster is a bit vector of length 65Kbit. Each bit of the vector represents
// a 16bit word. When a bit is set then this word may start an sync sequence.
// So, using just one table lookup we check a sequence of 2 bytes. This speeds
// up scanning a lot.
///////////////////////////////////////////////////////////////////////////////

void
SyncScan::build_booster(uint16_t word, int node, int depth)
{
  static const uint32_t mask_tbl[] =
  {
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffff0000, 0xff000000, 0xf0000000, 0xc0000000, 0x80000000
  };

  static const int count_tbl[] =
  { 2048, 1024, 512, 256, 128, 64, 32, 16, 8, 4, 2, 1, 1, 1, 1, 1 };

  if (depth >= 16)
  {
    booster[word >> 5] |= 0x80000000 >> (word & 0x1f);
    return;
  }

  if (node == SyncTrie::node_allow)
  {
    uint32_t mask = mask_tbl[depth] >> (word & 0x1f);
    int count = count_tbl[depth];
    for (int i = (word >> 5); i < (word >> 5) + count; i++)
      booster[i] = mask;
    return;
  }

  int l = graph.graph[node].left();
  int r = graph.graph[node].right();
  if (l != SyncTrie::node_deny) build_booster(word, l, depth + 1);
  if (r != SyncTrie::node_deny) build_booster(word | (0x8000 >> depth), r, depth + 1);
}

void
SyncScan::set_trie(const SyncTrie &gr)
{
  graph = gr;
  graph.optimize();
  memset(booster, 0, sizeof(booster));
  if (!graph.is_empty())
    build_booster(0, 0, 0);
}

SyncTrie
SyncScan::get_trie() const
{ return SyncTrie(graph); }

bool
SyncScan::scan_pos(const uint8_t *buf, size_t size, size_t &pos) const
{
  const size_t sync_size = graph.sync_size();

  if (pos > size || size - pos < sync_size)
    // We need more data, do nothing.
    return false;

  if (graph.is_empty())
  {
    pos = size;
    return false;
  }

  ///////////////////////////////////////////////////////
  // Scan using the booster

  if (size - pos >= 4)
  {
    uint32_t sync = (buf[pos] << 16) | (buf[pos+1] << 8) | buf[pos+2];
    size_t last = size;
    if (sync_size > 4)
      last = size - sync_size + 4;

    for (size_t i = pos + 3; i < last; i++)
    {
      sync = (sync << 8) | buf[i];
      if (booster[sync >> 21] & (0x80000000 >> ((sync >> 16) & 0x1f)))
        if (graph.is_sync(buf+i-3))
        {
          pos = i - 3;
          return true;
        }
    }
    pos = last - 3;
  }

  ///////////////////////////////////////////////////////
  // Scan last bytes without the booster (if nessesary)

  while (size - pos >= sync_size)
    if (graph.is_sync(buf + pos))
      return true;
    else
      pos++;

  return false;
}

bool
SyncScan::scan_shift(uint8_t *buf, size_t &size) const
{
  if (graph.is_empty())
  {
    size = 0;
    return false;
  }

  if (size < graph.sync_size())
    // We need more data, do nothing.
    return false;

  size_t pos = 0;
  bool result = scan_pos(buf, size, pos);
  memmove(buf, buf + pos, size - pos);
  size -= pos;
  return result;
}
