/**************************************************************************//**
  \file multi_fir.h
  \brief MultiFIR: Apply several FIR filters sequentially.
******************************************************************************/

#ifndef VALIB_MULTI_FIR_H
#define VALIB_MULTI_FIR_H

#include "../fir.h"

/**************************************************************************//**
  \class MultiFIR
  \brief Apply several FIR filters sequentially.

  Several filters applied sequentially may be represented as a single equivalent
  FIR filter. The reason to actually build this filter is the computational
  efficiency. Equivalent filter requires significantly less computations.

  For example, complexity of 2 FIR filters of length N when applied one-by-one
  is O(2 * log(N)) per output sample. Equivalent filter has complexity about
  O(log(2N)) = O(log(2) + log(N)).

  The filter is found as a convolution of originating filters. The resulting
  filter's length equals to sum_i(n_i-1) + 1 where n_i is a length of i-th
  filter.

  MultiFIR generator takes a list of generators as a parameter and builds
  a single filter from all of them. Version changes transparently with change
  of originating generators.

  Generator with empty list of generators produces no fir instance
  (make() returns null pointer).

  Null generator pointers in the list are ignored.

  Incorrect firs in the sequence (when generator returns null) are ignored.

  Zero filter in the sequence cleverly produces zero filter at output.

  Sequence of gain filters is a gain filter.

  \fn void MultiFIR::set(const FIRGen *const *list, size_t count)
    \param list List of originating generators
    \param count Number of generators

    Specify the list of filters co combine. Generators must live until release()
    call or MultiFIR destruction. Ownership is not taken.

  \fn void MultiFIR::release()
    Clear the list of filters.

******************************************************************************/

class MultiFIR : public FIRGen
{
protected:
  size_t count;        //!< Number of generators in list
  const FIRGen **list; //!< List of generators

  mutable int ver;
  mutable int list_ver;

public:
  MultiFIR();
  MultiFIR(const FIRGen *const *list, size_t count);
  ~MultiFIR();

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
