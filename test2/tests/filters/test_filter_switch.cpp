/*
  FilterSwitch test
*/

#include <boost/test/unit_test.hpp>
#include "filters/filter_switch.h"
#include "filters/passthrough.h"

class FilterSwitchMock : public Passthrough
{
public:
  FilterSwitchMock(Speakers accept_spk_):
  accept_spk(accept_spk_)
  {}

  virtual bool can_open(Speakers spk) const
  { return spk == accept_spk; }

protected:
  Speakers accept_spk;
};

BOOST_AUTO_TEST_SUITE(filter_switch)

BOOST_AUTO_TEST_CASE(constructor)
{
  FilterSwitch f;
}

BOOST_AUTO_TEST_CASE(open)
{
  const Speakers spk1(FORMAT_LINEAR, MODE_MONO, 48000);
  const Speakers spk2(FORMAT_LINEAR, MODE_STEREO, 48000);

  FilterSwitchMock f1(spk1);
  FilterSwitchMock f2(spk2);

  FilterSwitch::list_t list;
  list.push_back(&f1);
  list.push_back(&f2);

  FilterSwitch filter_switch(list);

  filter_switch.open(spk1);
  BOOST_CHECK(filter_switch.is_open());
  BOOST_CHECK(f1.is_open());
  BOOST_CHECK(!f2.is_open());

  filter_switch.open(spk2);
  BOOST_CHECK(filter_switch.is_open());
  BOOST_CHECK(!f1.is_open());
  BOOST_CHECK(f2.is_open());
}

BOOST_AUTO_TEST_SUITE_END()
