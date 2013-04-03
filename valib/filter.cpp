#include "log.h"
#include "filter.h"

string
Filter::name() const
{
  string type_name = typeid(*this).name();
  if (type_name.compare(0, 6, "class ") == 0)
    type_name.replace(0, 6, "");
  return type_name;
}

///////////////////////////////////////////////////////////////////////////////
// SimpleFilter

bool SimpleFilter::open(Speakers new_spk)
{
  const string func = string("open(") + new_spk.print() + string(")");
  valib_log(log_event, name(), func);

  if (!can_open(new_spk))
  {
    valib_log(log_error, name(), func + " failed because can_open() returns false");
    return false;
  }

  spk = new_spk;
  if (!init())
  {
    spk = spk_unknown;
    valib_log(log_error, name(), func + " failed because init() returns false");
    return false;
  }

  f_open = true;
  valib_log(log_event, name(), func + " succeeded");
  return true;
}

void SimpleFilter::close()
{
  valib_log(log_event, name(), "close()");
  uninit();
  f_open = false;
  spk = spk_unknown;
}

void SimpleFilter::reset()
{
  valib_log(log_event, name(), "reset()");
}