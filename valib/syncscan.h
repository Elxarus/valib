/**************************************************************************//**
  \file syncscan.h
  \brief SyncTrie: The definition of a sync point.
         SyncScan: Scanning algorithm using a trie.
******************************************************************************/

#ifndef VALIB_SYNC_TRIE_H
#define VALIB_SYNC_TRIE_H

#include <vector>
#include <boost/operators.hpp>
#include "defs.h"

/**************************************************************************//**
  \class SyncTrie
  \brief The definition of a sync point.

  Holds a set of bit sequences that we search for during the synchronization.
  As named, it is a trie, that allows fast seach.

  \fn SyncTrie::SyncTrie()
    Construct an empty trie. We'll never sync using an empty graph.

  \fn SyncTrie::SyncTrie(uint32_t value, unsigned bits);
    Construct a graph that holds a value 'value' of length 'bits'.

  \fn SyncTrie::SyncTrie(unsigned bits);
    Construct a trie that holds all values of length 'bits'. The main purpose
    for such trie is to represent 'any value' in the middle of a sync sequence.

  \fn void SyncTrie::clear();
    Drop all data from the trie. It becomes empty.

  \fn void SyncTrie::invert();
    Inverse the trie.

    Example:
    SyncTrie t(0x55, 8); // trie for value 0x55
    t.invert();          // sync on anything except 0x55

  \fn void SyncTrie::merge(const SyncTrie &other);
    Add sync sequences from another trie. The resulting trie unions both
    sets of sync sequences.

    Example:
    SyncTrie t1(0x55, 8); // trie for value 0x55
    SyncTrie t2(0xaa, 8); // trie for value 0xaa
    t1.merge(t2);         // sync on 0x55 or 0xaa

  \fn void SyncTrie::append(const SyncTrie &other);
    Append the trie to the end of the current trie.

    Example 1:
    SyncTrie t1(0x55, 8); // trie for value 0x55
    SyncTrie t2(0xaa, 8); // trie for value 0xaa
    t1.append(t2);        // sync on the sequence 0x55 0xaa

    Example 2:
    SyncTrie t1(0x55, 8); // trie for value 0x55
    SyncTrie t2(0xaa, 8); // trie for value 0xaa
    SyncTrie t3(0x01, 8); // trie for value 0xaa
    t1.merge(t2);         // sync on 0x55 or 0xaa
    t1.append(t3);        // sync on sequences 0x55 0x01 or 0xaa 0x01

  \fn void SyncTrie::optimize();
    Optimizes the trie.

  \fn bool SyncTrie::is_empty() const;
    Returns true when the trie is empty.

  \fn size_t SyncTrie::sync_size() const;
    Length of the longest sync sequence in bytes.

  \fn bool SyncTrie::is_sync(const uint8_t *buf) const;
    Returns true when 'buf' contains a sync sequence.
    'buf' must be at least sync_size() long.

******************************************************************************/

class SyncTrie :
  boost::addable<SyncTrie>,
  boost::orable<SyncTrie>
{
protected:
  enum { node_allow = -1, node_deny = 0 };

  struct Node {
    int children[2];

    Node()
    {
      children[0] = node_deny;
      children[1] = node_deny;
    }

    Node(int l, int r)
    {
      children[0] = l;
      children[1] = r;
    }

    inline int &left()  { return children[0]; }
    inline int &right() { return children[1]; }
    inline const int &left()  const { return children[0]; }
    inline const int &right() const { return children[1]; }
  };

  typedef std::vector<Node> Graph;

  Graph graph;
  int depth;

  static int insert_node(Graph &gr);
  static int copy(Graph &result, const Graph &gr, int node);
  static int find_depth(Graph &gr, int node);
  static int find_depth(Graph &gr);
  static int merge(Graph &result, const Graph &gr1, int node1, const Graph &gr2, int node2);
  static int optimize(Graph &result, const Graph &gr, int node);

  void serialize(std::string &result, int node);
  int  deserialize(const std::string &s, size_t &pos, int &depth);

  friend class SyncScan;

public:
  static const SyncTrie any;
  static const SyncTrie one;
  static const SyncTrie zero;

  SyncTrie(): depth(0) {}
  SyncTrie(uint32_t value, unsigned bits);
  explicit SyncTrie(unsigned bits);
  explicit SyncTrie(const std::string &trie);

  void clear();
  void invert();
  void merge(const SyncTrie &other);
  void append(const SyncTrie &other);
  void optimize();

  SyncTrie &operator |=(const SyncTrie &other)
  { merge(other); return *this; }

  SyncTrie &operator +=(const SyncTrie &other)
  { append(other); return *this; }

  SyncTrie operator ~()
  {
    SyncTrie result(*this);
    result.invert();
    return result;
  }

  inline bool is_empty() const
  { return graph.size() == 0; }

  inline size_t sync_size() const
  { return (depth + 7) >> 3; }

  inline bool is_sync(const uint8_t *buf) const
  {
    int node = 0;
    while (true)
    {
      uint8_t test = *buf++;
      node = graph[node].children[(test >> 7) & 1];
      if (node == node_allow) return true;
      if (node == node_deny) return false;
      node = graph[node].children[(test >> 6) & 1];
      if (node == node_allow) return true;
      if (node == node_deny) return false;
      node = graph[node].children[(test >> 5) & 1];
      if (node == node_allow) return true;
      if (node == node_deny) return false;
      node = graph[node].children[(test >> 4) & 1];
      if (node == node_allow) return true;
      if (node == node_deny) return false;
      node = graph[node].children[(test >> 3) & 1];
      if (node == node_allow) return true;
      if (node == node_deny) return false;
      node = graph[node].children[(test >> 2) & 1];
      if (node == node_allow) return true;
      if (node == node_deny) return false;
      node = graph[node].children[(test >> 1) & 1];
      if (node == node_allow) return true;
      if (node == node_deny) return false;
      node = graph[node].children[(test >> 0) & 1];
      if (node == node_allow) return true;
      if (node == node_deny) return false;
    }
  }

  void print(int node = 0);

  std::string serialize()
  {
    std::string result;
    if (!is_empty())
      serialize(result, 0);
    return result;
  }

  bool deserialize(const std::string &str)
  {
    clear();
    try {
      size_t pos = 0;
      deserialize(str, pos, depth);
      return true;
    }
    catch (...) {
      clear();
      return false;
    }
  }

  int get_depth() const { return depth; }
  size_t get_size() const { return graph.size(); }
};

/**************************************************************************//**
  \class SyncScan
  \brief Scanning algorithm using SyncTrie.

  \fn SyncScan::SyncScan()
    Constructs an uninitialized scanner.

  \fn SyncScan::SyncScan(const SyncTrie &gr);
    Construct and initialize the scanner with the trie specified.

  \fn void SyncScan::clear()
    Drop the trie from the scanner.

  \fn void SyncScan::init(const SyncTrie &gr)
    Init the scanner using the trie specified.

  \fn bool SyncScan::scan(const uint8_t *buf, size_t size, size_t &pos) const
    Scan the buffer 'buf' of size 'size', starting from the position 'pos'.

    When scanner finds a syncpoint, it returns true and sets 'pos' to the
    position of the syncpoint found. Otherwise, it returns false and sets
    'pos' to the position size - trie.sync_size() + 1. Note, that last
    trie.sync_size() - 1 bytes may belong to a syncpoint, but we cannot check
    this because we need more data to continue scanning. Therefore, if you
    have more data to scan, you have to save this bytes.
******************************************************************************/

class SyncScan
{
protected:
  SyncTrie graph;
  uint32_t booster[2048];
  void build_booster(uint16_t word, int node, int depth);

public:
  SyncScan()
  {}
  SyncScan(const SyncTrie &t)
  { init(t); }

  void clear();
  void init(const SyncTrie &t);
  bool scan(const uint8_t *buf, size_t size, size_t &pos) const;
};

#endif
