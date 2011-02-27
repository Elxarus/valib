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

static const Speakers spk1(FORMAT_LINEAR, MODE_MONO, 48000);
static const Speakers spk2(FORMAT_LINEAR, MODE_STEREO, 48000);

BOOST_AUTO_TEST_SUITE(filter_switch)

BOOST_AUTO_TEST_CASE(constructor)
{
  FilterSwitch f;
  BOOST_CHECK_EQUAL(f.get_filters().size(), 0);
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  FilterSwitchMock f1(spk1);
  FilterSwitchMock f2(spk2);

  FilterSwitch::list_t list;
  list.push_back(&f1);
  list.push_back(&f2);

  FilterSwitch f(list);
  BOOST_CHECK_EQUAL(f.get_filters().size(), 2);
  BOOST_CHECK_EQUAL(f.get_filters()[0], &f1);
  BOOST_CHECK_EQUAL(f.get_filters()[1], &f2);
  BOOST_CHECK(f.can_open(spk1));
  BOOST_CHECK(f.can_open(spk2));
}

BOOST_AUTO_TEST_CASE(set_release)
{
  FilterSwitchMock f1(spk1);
  FilterSwitchMock f2(spk2);

  FilterSwitch::list_t list;
  list.push_back(&f1);
  list.push_back(&f2);

  FilterSwitch f;
  f.set_filters(list);
  BOOST_CHECK_EQUAL(f.get_filters().size(), 2);
  BOOST_CHECK_EQUAL(f.get_filters()[0], &f1);
  BOOST_CHECK_EQUAL(f.get_filters()[1], &f2);
  BOOST_CHECK(f.can_open(spk1));
  BOOST_CHECK(f.can_open(spk2));

  f.release_filters();
  BOOST_CHECK_EQUAL(f.get_filters().size(), 0);
  BOOST_CHECK(!f.can_open(spk1));
  BOOST_CHECK(!f.can_open(spk2));
}

BOOST_AUTO_TEST_CASE(add_remove)
{
  FilterSwitchMock f1(spk1);
  FilterSwitchMock f2(spk2);
  FilterSwitch f;

  f.add_filter(&f1);
  BOOST_CHECK_EQUAL(f.get_filters().size(), 1);
  BOOST_CHECK(f.can_open(spk1));
  BOOST_CHECK(!f.can_open(spk2));

  f.add_filter(&f2);
  BOOST_CHECK_EQUAL(f.get_filters().size(), 2);
  BOOST_CHECK(f.can_open(spk1));
  BOOST_CHECK(f.can_open(spk2));

  f.remove_filter(&f1);
  BOOST_CHECK_EQUAL(f.get_filters().size(), 1);
  BOOST_CHECK(!f.can_open(spk1));
  BOOST_CHECK(f.can_open(spk2));

  f.remove_filter(&f2);
  BOOST_CHECK_EQUAL(f.get_filters().size(), 0);
  BOOST_CHECK(!f.can_open(spk1));
  BOOST_CHECK(!f.can_open(spk2));
}

BOOST_AUTO_TEST_CASE(open)
{
  FilterSwitchMock f1(spk1);
  FilterSwitchMock f2(spk2);

  FilterSwitch::list_t list;
  list.push_back(&f1);
  list.push_back(&f2);

  FilterSwitch f(list);

  f.open(spk1);
  BOOST_CHECK(f.is_open());
  BOOST_CHECK(f1.is_open());
  BOOST_CHECK(!f2.is_open());

  f.open(spk2);
  BOOST_CHECK(f.is_open());
  BOOST_CHECK(!f1.is_open());
  BOOST_CHECK(f2.is_open());
}

BOOST_AUTO_TEST_SUITE_END()
