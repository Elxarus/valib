#include "filter2.h"

Filter *Filter2::filter()
{
  if (!thunk)
    thunk = new FilterThunk(this);
  return thunk;
};

Filter2::~Filter2()
{
  safe_delete(thunk);
}
