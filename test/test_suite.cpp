/*
  Test for the test suite
*/

#include "suite.h"

///////////////////////////////////////////////////////////////////////////////
// I want to be able to write tests without any additional writing
// (class definition, etc): just say that "I start writing a test here".
// There're two examples below: one always passes and other fails. Later we
// will try to call this tests...

TEST(plain_success, "Plain success test")
  CHECK(true);
TEST_END(plain_success);

TEST(plain_fail, "Plain failure test")
  CHECK(false);
TEST_END(plain_fail);

///////////////////////////////////////////////////////////////////////////////
// But also I wish wo be able to write complex tests with setup, cleanup and
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
      CHECK(sub_test2(i).failed());
    was_run_flag = true;
    return test_passed;
  }

  TestResult sub_test1()
  {
    log->msg("Sub test 1");
    CHECK(true);
    return test_passed;
  }

  TestResult sub_test2(int param)
  {
    log->msg("Sub test 2 with parameter %i", param);
    CHECK(true);
    return test_passed;
  }

public:
  WasRun(): Test("WasRun", "Test class test"), was_run_flag(false)
  {}

  bool was_run() const { return was_run_flag; }
};


///////////////////////////////////////////////////////////////////////////////
// Actual test

TEST(suite_test, "Test suite self-test")
  Test *t;

  // I don't want to pollute the main log with results of
  // test calls, so I need a dummy log with no output

  Log dummy_log(0);

  // Ensure that a test actualy runs

  WasRun was_run;
  CHECK(was_run.was_run() == false);
  CHECK(was_run.run(&dummy_log).passed());
  CHECK(was_run.was_run() == true);

  // Run plain tests

  t = CREATE_TEST(plain_success);
  CHECK(t->run(&dummy_log).passed());
  delete t;

  t = CREATE_TEST(plain_fail);
  CHECK(t->run(&dummy_log).failed());
  delete t;

TEST_END(suite_test);
