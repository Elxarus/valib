/**************************************************************************//**
  \file parallel_fir.h
  \brief ParallelFIR: Combines several filters into one in parallel.
******************************************************************************/

#ifndef VALIB_PARALLEL_FIR_H
#define VALIB_PARALLEL_FIR_H

#include "../fir.h"

/**************************************************************************//**
  \class ParallelFIR
  \brief Combines several filters into one in parallel.

  When several filters are applied to the same signal and result is summed, it
  may be represented as a single equivalent FIR filter. The reason to actually
  build this filter is the computational efficiency. Equivalent filter requires
  significantly less computations. In conjunction with MultiFIR it allows to
  build complex and efficient filters from simple primitives.

  \verbatim
        +----------+
    +-->| Filter 1 |-----+
    |   +----------+     |
    |                    v
    |   +----------+   +---+
  --+-->| Filter 2 |-->|sum|--->
    |   +----------+   +---+
    |                    ^
    |      .....         |
    |                    |
    |   +----------+     |
    +-->| Filter n |-----+
        +----------+
  \endverbatim

  The filter is found as a sum of originating filters. Filters are aligned to
  match centers before summing. The resulting filter's length equals to
  max(c_i) + max(n_i-c_i) where n_i is a length of i-th filter and c_i is a
  position of the center of the filter. 

  ParallelFIR generator takes a list of generators as a parameter and builds
  a single filter from all of them. Version changes transparently with change
  of originating generators.

  Generator with empty list of generators produces no fir instance
  (make() returns null pointer).

  Null generator pointers in the list are ignored.

  Incorrect firs in the sequence (when generator returns null) are ignored.

  \fn void ParallelFIR::set(const FIRGen *const *list, size_t count)
    \param list List of originating generators
    \param count Number of generators

    Specify the list of filters co combine. Generators must live until release()
    call or ParallelFIR destruction. Ownership is not taken.

  \fn void ParallelFIR::release()
    Clear the list of filters.
******************************************************************************/

class ParallelFIR : public FIRGen
{
protected:
  size_t count;
  const FIRGen **list;

  mutable int ver;
  mutable int list_ver;

public:
  ParallelFIR();
  ParallelFIR(const FIRGen *const *list, size_t count);
  ~ParallelFIR();

  /////////////////////////////////////////////////////////
  // Own interface

  void set(const FIRGen *const *list, size_t count);
  void release();

  /////////////////////////////////////////////////////////
  // FIRGen interface

  virtual int version() const;
  virtual const FIRInstance *make(int sample_rate) const;

};

#endif
