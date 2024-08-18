#include "munit.h"
#include <stdint.h>
#include <stdbool.h>
#include "rp2040_tests.h"

// Creating a test suite is pretty simple.  First, you'll need an
// array of tests:
static MunitTest all_tests[] = {
  {
    // The name is just a unique human-readable way to identify the
    // test. You can use it to run a specific test if you want, but
    // usually it's mostly decorative.
    (char*) "/rp2040/swdv2",
    // You probably won't be surprised to learn that the tests are
    // functions.
    test_swd_v2,
    // If you want, you can supply a function to set up a fixture.  If
    // you supply NULL, the user_data parameter from munit_suite_main
    // will be used directly.  If, however, you provide a callback
    // here the user_data parameter will be passed to this callback,
    // and the return value from this callback will be passed to the
    // test function.
    //
    // For our example we don't really need a fixture, but lets
    // provide one anyways.
    rp2040_setup,
    // If you passed a callback for the fixture setup function, you
    // may want to pass a corresponding callback here to reverse the
    // operation.
    NULL,
    // Finally, there is a bitmask for options you can pass here.  You
    // can provide either MUNIT_TEST_OPTION_NONE or 0 here to use the
    // defaults.
    MUNIT_TEST_OPTION_NONE,
    NULL
  },
  // Usually this is written in a much more compact format; all these
  // comments kind of ruin that, though.  Here is how you'll usually
  // see entries written:

  { (char*) "/rp2040/core_id",       test_get_core_id,           rp2040_setup,     NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/rp2040/apsel",         test_get_apsel,             rp2040_setup,     NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/rp2040/target_info",   test_target_info,           rp2040_setup,     NULL, MUNIT_TEST_OPTION_NONE, NULL },

  // new test go here

  // To tell the test runner when the array is over, just add a NULL
  // entry at the end. */
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

/* If you wanted to have your test suite run other test suites you
 * could declare an array of them.  Of course each sub-suite can
 * contain more suites, etc. */
/* static const MunitSuite other_suites[] = { */
/*   { "/second", test_suite_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE }, */
/*   { NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE } */
/* }; */

/* Now we'll actually declare the test suite.  You could do this in
 * the main function, or on the heap, or whatever you want. */
static const MunitSuite test_suite = {
  /* This string will be prepended to all test names in this suite;
   * for example, "/example/rand" will become "/µnit/example/rand".
   * Note that, while it doesn't really matter for the top-level
   * suite, NULL signal the end of an array of tests; you should use
   * an empty string ("") instead. */
  (char*) "",
  /* The first parameter is the array of test suites. */
  all_tests,
  /* In addition to containing test cases, suites can contain other
   * test suites.  This isn't necessary in this example, but it can be
   * a great help to projects with lots of tests by making it easier
   * to spread the tests across many files.  This is where you would
   * put "other_suites" (which is commented out above). */
  NULL,
  /* An interesting feature of µnit is that it supports automatically
   * running multiple iterations of the tests.  This is usually only
   * interesting if you make use of the PRNG to randomize your tests
   * cases a bit, or if you are doing performance testing and want to
   * average multiple runs.  0 is an alias for 1. */
  1,
  /* Just like MUNIT_TEST_OPTION_NONE, you can provide
   * MUNIT_SUITE_OPTION_NONE or 0 to use the default settings. */
  MUNIT_SUITE_OPTION_NONE
};

/* This is only necessary for EXIT_SUCCESS and EXIT_FAILURE, which you
 * *should* be using but probably aren't (no, zero and non-zero don't
 * always mean success and failure).  I guess my point is that nothing
 * about µnit requires it. */
#include <stdlib.h>

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
  /* Finally, we'll actually run our test suite!  That second argument
   * is the user_data parameter which will be passed either to the
   * test or (if provided) the fixture setup function. */
  return munit_suite_main(&test_suite, (void*) "µnit", argc, argv);
}
