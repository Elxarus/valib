#include <boost/test/unit_test.hpp>
#include "defs.h"
#include "vargs.h"

BOOST_AUTO_TEST_SUITE(vargs)

BOOST_AUTO_TEST_CASE(is_option)
{
  // empty argument
  BOOST_CHECK(!arg_t("").is_option("x", argt_exist));
  BOOST_CHECK(!arg_t("").is_option("x", argt_bool));
  BOOST_CHECK(!arg_t("").is_option("x", argt_int));
  BOOST_CHECK(!arg_t("").is_option("x", argt_double));
  BOOST_CHECK(!arg_t("").is_option("x", argt_text));
  BOOST_CHECK(!arg_t("").is_option("x", argt_enum));
  // no value
  BOOST_CHECK(arg_t("-x").is_option("x", argt_exist));
  BOOST_CHECK(arg_t("-x").is_option("x", argt_bool));
  BOOST_CHECK(!arg_t("-x").is_option("x", argt_int));
  BOOST_CHECK(!arg_t("-x").is_option("x", argt_double));
  BOOST_CHECK(!arg_t("-x").is_option("x", argt_text));
  BOOST_CHECK(!arg_t("-x").is_option("x", argt_enum));
  // empty value
  BOOST_CHECK(!arg_t("-x:").is_option("x", argt_exist));
  BOOST_CHECK(arg_t("-x:").is_option("x", argt_bool));
  BOOST_CHECK(arg_t("-x:").is_option("x", argt_int));
  BOOST_CHECK(arg_t("-x:").is_option("x", argt_double));
  BOOST_CHECK(arg_t("-x:").is_option("x", argt_text));
  BOOST_CHECK(arg_t("-x:").is_option("x", argt_enum));
  BOOST_CHECK(!arg_t("-x=").is_option("x", argt_exist));
  BOOST_CHECK(arg_t("-x=").is_option("x", argt_bool));
  BOOST_CHECK(arg_t("-x=").is_option("x", argt_int));
  BOOST_CHECK(arg_t("-x=").is_option("x", argt_double));
  BOOST_CHECK(arg_t("-x=").is_option("x", argt_text));
  BOOST_CHECK(arg_t("-x=").is_option("x", argt_enum));
  // non-empty value
  BOOST_CHECK(!arg_t("-x:value").is_option("x", argt_exist));
  BOOST_CHECK(arg_t("-x:value").is_option("x", argt_bool));
  BOOST_CHECK(arg_t("-x:value").is_option("x", argt_int));
  BOOST_CHECK(arg_t("-x:value").is_option("x", argt_double));
  BOOST_CHECK(arg_t("-x:value").is_option("x", argt_text));
  BOOST_CHECK(arg_t("-x:value").is_option("x", argt_enum));
  BOOST_CHECK(!arg_t("-x=value").is_option("x", argt_exist));
  BOOST_CHECK(arg_t("-x=value").is_option("x", argt_bool));
  BOOST_CHECK(arg_t("-x=value").is_option("x", argt_int));
  BOOST_CHECK(arg_t("-x=value").is_option("x", argt_double));
  BOOST_CHECK(arg_t("-x=value").is_option("x", argt_text));
  BOOST_CHECK(arg_t("-x=value").is_option("x", argt_enum));
  // wrong name with no value
  BOOST_CHECK(!arg_t("-xx").is_option("x", argt_exist));
  BOOST_CHECK(!arg_t("-xx").is_option("x", argt_bool));
  BOOST_CHECK(!arg_t("-xx").is_option("x", argt_int));
  BOOST_CHECK(!arg_t("-xx").is_option("x", argt_double));
  BOOST_CHECK(!arg_t("-xx").is_option("x", argt_text));
  BOOST_CHECK(!arg_t("-xx").is_option("x", argt_enum));
  // wrong name with value
  BOOST_CHECK(!arg_t("-xx:value").is_option("x", argt_exist));
  BOOST_CHECK(!arg_t("-xx:value").is_option("x", argt_bool));
  BOOST_CHECK(!arg_t("-xx:value").is_option("x", argt_int));
  BOOST_CHECK(!arg_t("-xx:value").is_option("x", argt_double));
  BOOST_CHECK(!arg_t("-xx:value").is_option("x", argt_text));
  BOOST_CHECK(!arg_t("-xx:value").is_option("x", argt_enum));
  // no starting '-' sign
  BOOST_CHECK(!arg_t("x").is_option("x", argt_exist));
  BOOST_CHECK(!arg_t("x").is_option("x", argt_bool));
  BOOST_CHECK(!arg_t("x").is_option("x", argt_int));
  BOOST_CHECK(!arg_t("x").is_option("x", argt_double));
  BOOST_CHECK(!arg_t("x").is_option("x", argt_text));
  BOOST_CHECK(!arg_t("x").is_option("x", argt_enum));
  // wrong starting sign
  BOOST_CHECK(!arg_t("+x").is_option("x", argt_exist));
  BOOST_CHECK(!arg_t("+x").is_option("x", argt_bool));
  BOOST_CHECK(!arg_t("+x").is_option("x", argt_int));
  BOOST_CHECK(!arg_t("+x").is_option("x", argt_double));
  BOOST_CHECK(!arg_t("+x").is_option("x", argt_text));
  BOOST_CHECK(!arg_t("+x").is_option("x", argt_enum));

  // special: boolean argument with +/- sign at the end
  BOOST_CHECK(arg_t("-x+").is_option("x", argt_bool));
  BOOST_CHECK(arg_t("-x-").is_option("x", argt_bool));
  BOOST_CHECK(!arg_t("-x*").is_option("x", argt_bool));
  BOOST_CHECK(!arg_t("-x++").is_option("x", argt_bool));
}

BOOST_AUTO_TEST_CASE(as_bool)
{
  BOOST_CHECK_EQUAL(arg_t("-x").as_bool(), true);
  BOOST_CHECK_EQUAL(arg_t("-x+").as_bool(), true);
  BOOST_CHECK_EQUAL(arg_t("-x:+").as_bool(), true);
  BOOST_CHECK_EQUAL(arg_t("-x=+").as_bool(), true);
  BOOST_CHECK_EQUAL(arg_t("-x:1").as_bool(), true);
  BOOST_CHECK_EQUAL(arg_t("-x:yEs").as_bool(), true);
  BOOST_CHECK_EQUAL(arg_t("-x:tRuE").as_bool(), true);

  BOOST_CHECK_EQUAL(arg_t("-x-").as_bool(), false);
  BOOST_CHECK_EQUAL(arg_t("-x:-").as_bool(), false);
  BOOST_CHECK_EQUAL(arg_t("-x=-").as_bool(), false);
  BOOST_CHECK_EQUAL(arg_t("-x:0").as_bool(), false);
  BOOST_CHECK_EQUAL(arg_t("-x:nO").as_bool(), false);
  BOOST_CHECK_EQUAL(arg_t("-x:fAlSe").as_bool(), false);

  BOOST_CHECK_THROW(arg_t("-x:++").as_bool(), arg_t::bad_value_e);
  BOOST_CHECK_THROW(arg_t("-x:wrong").as_bool(), arg_t::bad_value_e);
}

BOOST_AUTO_TEST_CASE(as_int)
{
  BOOST_CHECK_EQUAL(arg_t("-x:0").as_int(), 0);
  BOOST_CHECK_EQUAL(arg_t("-x:10").as_int(), 10);
  BOOST_CHECK_EQUAL(arg_t("-x:-10").as_int(), -10);
  BOOST_CHECK_EQUAL(arg_t("-x:0xf0").as_int(), 240);
  BOOST_CHECK_EQUAL(arg_t("-x=10").as_int(), 10);

  BOOST_CHECK_THROW(arg_t("-x:wrong").as_int(), arg_t::bad_value_e);
  BOOST_CHECK_THROW(arg_t("-x:1.0").as_int(), arg_t::bad_value_e);
}

BOOST_AUTO_TEST_CASE(as_double)
{
  BOOST_CHECK_EQUAL(arg_t("-x:0").as_double(), 0);
  BOOST_CHECK_EQUAL(arg_t("-x:10.5").as_double(), 10.5);
  BOOST_CHECK_EQUAL(arg_t("-x:-10.5").as_double(), -10.5);
  BOOST_CHECK_EQUAL(arg_t("-x:+2.5e-2").as_double(), 2.5e-2);
  BOOST_CHECK_EQUAL(arg_t("-x=10.5").as_double(), 10.5);

  BOOST_CHECK_THROW(arg_t("-x:wrong").as_double(), arg_t::bad_value_e);
  BOOST_CHECK_THROW(arg_t("-x:1.0t").as_double(), arg_t::bad_value_e);
}

BOOST_AUTO_TEST_CASE(as_text)
{
  BOOST_CHECK_EQUAL(arg_t("-x:").as_text(), std::string(""));
  BOOST_CHECK_EQUAL(arg_t("-x:test").as_text(), std::string("test"));
}

BOOST_AUTO_TEST_CASE(choose)
{
  const enum_opt options[] =
  {
    { "option1", 1 },
    { "option2", 2 },
    { "option3", 3 },
  };

  BOOST_CHECK_EQUAL(arg_t("-x:option1").choose(options, array_size(options)), 1);
  BOOST_CHECK_EQUAL(arg_t("-x:option2").choose(options, array_size(options)), 2);
  BOOST_CHECK_EQUAL(arg_t("-x:option3").choose(options, array_size(options)), 3);

  BOOST_CHECK_THROW(arg_t("-x:Option1").choose(options, array_size(options)), arg_t::bad_value_e);

  BOOST_CHECK_EQUAL(arg_t("-x:oPtIoN1").choose_lowcase(options, array_size(options)), 1);
  BOOST_CHECK_EQUAL(arg_t("-x:OpTiOn2").choose_lowcase(options, array_size(options)), 2);
  BOOST_CHECK_EQUAL(arg_t("-x:oPtIoN3").choose_lowcase(options, array_size(options)), 3);

  BOOST_CHECK_THROW(arg_t("-x:option4").choose_lowcase(options, array_size(options)), arg_t::bad_value_e);
}

BOOST_AUTO_TEST_SUITE_END()
