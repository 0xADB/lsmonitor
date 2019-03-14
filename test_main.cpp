#include <string>
#include <sstream>

using namespace std::string_literals;

#define BOOST_TEST_MODULE test_main

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(test_suite_main)

  BOOST_AUTO_TEST_CASE(test_processor_with_plain_commands)
  {
    BOOST_CHECK(true);
  }

BOOST_AUTO_TEST_SUITE_END()
