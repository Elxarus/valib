#include "filter2.h"

Filter *Filter2::as_filter()
{ return thunk; }

Filter2::operator Filter *()
{ return thunk; }

Filter *Filter2::operator ->()
{ return thunk; }

Filter2::operator const Filter *() const
{ return thunk; }

const Filter *Filter2::operator ->() const
{ return thunk; }

Filter2::Filter2()
{
  thunk = new FilterThunk(this);
}

Filter2::~Filter2()
{
  safe_delete(thunk);
}
