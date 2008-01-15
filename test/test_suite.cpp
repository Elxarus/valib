/*
  Test for the test suite
*/

#include "suite.h"

///////////////////////////////////////////////////////////////////////////////
// I want to be able to write tests without any additional writing
// (class definition, etc): just say that "I start writing a test here".
// There're two examples of such plain tests below: one always passes and
// another fails. Later we will try to call this tests in different ways.

TEST(plain_pass, "Plain pass test")
  CHECK(true);
TEST_END(plain_pass);

TEST(plain_fail, "Plain failure test")
  CHECK(false);
TEST_END(plain_fail);

///////////////////////////////////////////////////////////////////////////////
// But also I wish to be able to write complex tests with setup, cleanup and
// set of parametrized sub-tests. Here is an example. Also, this test ensures
// that the test function actually runs. We cannot do this with plain test
// because it does not keep any state after completition.

class WasRun : public Test
{
protected:
  bool was_run_flag;

  TestResult do_test()
  {
    CHECK(sub_test1().passed());
    for (int i = 0; i < 3; i++)
      CHECK(sub_test2(i).passed());

    was_run_flag = true;
    return test_passed;
  }

  TestResult sub_test1()
  {
    log->msg("Subtest 1");
    CHECK(true);
    return test_passed;
  }

  TestResult sub_test2(int param)
  {
    log->msg("Subtest 2 with parameter %i", param);
    CHECK(true);
    return test_passed;
  }

public:
  WasRun(): Test("was_run", "Test example"), was_run_flag(false)
  {}

  bool was_run() const { return was_run_flag; }
};

extern "C" Test *was_run_factory() { return new WasRun(); }

///////////////////////////////////////////////////////////////////////////////
// Well, now it's time to collect tests into the suite.
// Suite uses test factories to create test objects. For plain test deafult
// factory can be obtained using TEST_FACTORY

SUITE(suite_pass, "Suite with successive tests only")
  TEST_FACTORY(plain_pass),
  was_run_factory,
SUITE_END;

SUITE(suite_fail, "Suite with fail test")
  TEST_FACTORY(plain_pass),
  TEST_FACTORY(plain_fail),
SUITE_END;

///////////////////////////////////////////////////////////////////////////////
// Actual test

TEST(suite_test, "Test suite self-test")
  Test *test;
  Test *suite;
  test_factory *f;

  // I don't want to pollute the main log with results of
  // test calls, so I need a dummy log with no output

  Log dummy_log(0);

  // Ensure that a test actualy runs

  WasRun was_run;
  CHECK(was_run.was_run() == false);
  CHECK(was_run.run(&dummy_log).passed());
  CHECK(was_run.was_run() == true);

  // Run plain tests (pass and fail)
  // Create tests using CREATE_TEST (only for plain tests)

  test = CREATE_TEST(plain_pass);
  CHECK(test->run(&dummy_log).passed());
  delete test;

  test = CREATE_TEST(plain_fail);
  CHECK(test->run(&dummy_log).failed());
  delete test;

  // Create a test using test factory

  f = TEST_FACTORY(plain_pass);
  test = f();
  CHECK(test->run(&dummy_log).passed());
  delete test;

  // Call tests from a suite
  // Create suites using CREATE_SUITE

  suite = CREATE_SUITE(suite_pass);
  CHECK(suite->run(&dummy_log).passed());
  delete suite;

  suite = CREATE_SUITE(suite_fail);
  CHECK(suite->run(&dummy_log).failed());
  delete suite;

  // Create a suite using test factory

  f = SUITE_FACTORY(suite_pass);
  suite = f();
  CHECK(suite->run(&dummy_log).passed());
  delete suite;

  // Search a test in a suite

  suite = CREATE_SUITE(suite_fail);
  test = suite->find("suite_fail"); // find the suite itself
  CHECK(test == suite);

  test = suite->find("plain_pass");
  CHECK(test != 0);
  CHECK(test->run(&dummy_log).passed());

  test = suite->find("plain_fail");
  CHECK(test != 0);
  CHECK(test->run(&dummy_log).failed());

  test = suite->find("fake");
  CHECK(test == 0);
  delete suite;

TEST_END(suite_test);
