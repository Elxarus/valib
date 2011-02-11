/**************************************************************************//**
  \file filter_switch.h
  \brief FilterSwitch: Choose a filter based on the input format
******************************************************************************/

#ifndef VALIB_FILTER_SWITCH_H
#define VALIB_FILTER_SWITCH_H

#include <vector>
#include "../filter.h"

/**************************************************************************//**
  \class FilterSwitch
  \brief Choose a filter based on the input format.

  When you have several filters that support different formats and you need to
  choose one depending on the format, FilterSwitch may be used.

  FilterSwitch accepts a list of filters and chooses one on open. All filter
  calls are passed to the filter selected.

  When FilterWrapper is open and you open it with a different format, the
  currently choosen filter is closed.

  <b>Example:</b>
  \code
  Filter1 f1;
  Filter2 f2;
  FilterSwitch::list_t list;
  list.push_back(&f1);
  list.push_back(&f2);

  FilterSwitch filter_switch(list);
  filter_switch.open(spk1); // Now filter is choosen and open.
  ...
  filter_switch.process(in, out); // Call is passed to the corrent filter
  ...
  filter_switch.open(spk2); // Old filter is closed and
                            // another filter is selected and open
  \endcode

  \fn void FilterSwitch::set_filters(const std::vector<Filter *> &new_filters)
  Sets the list of filters. If filter is open, the currently selected filter
  does not change.

  \fn std::vector<Filter *> FilterSwitch::get_filters() const
  Returns the list of filters.
******************************************************************************/

class FilterSwitch : public FilterWrapper
{
public:
  typedef std::vector<Filter *> list_t;

  FilterSwitch()
  {}

  FilterSwitch(const list_t &new_filters)
  { set_filters(new_filters); }

  void set_filters(const list_t &new_filters)
  { filters = new_filters; }

  list_t get_filters() const
  { return filters; }

  /////////////////////////////////////////////////////////
  // Fitler interface

  virtual bool can_open(Speakers spk) const
  {
    for (size_t i = 0; i < filters.size(); i++)
      if (filters[i] && filters[i]->can_open(spk))
        return true;
    return false;
  }

  virtual bool open(Speakers spk)
  {
    close();

    for (size_t i = 0; i < filters.size(); i++)
      if (filters[i] && filters[i]->can_open(spk))
        if (filters[i]->open(spk))
        {
          wrap(filters[i]);
          return true;
        }

    return false;
  }

  virtual void close()
  {
    if (FilterWrapper::is_open())
      FilterWrapper::close();
    unwrap();
  }

protected:
  list_t filters;
};

#endif
