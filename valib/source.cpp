#include "source.h"

string
Source2::name() const
{
  string type_name = typeid(*this).name();
  if (type_name.compare(0, 6, "class ") == 0)
    type_name.replace(0, 6, "");
  return type_name;
}
