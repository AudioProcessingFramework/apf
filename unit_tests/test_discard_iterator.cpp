#include "apf/iterator.h"  // for discard_iterator

#include "catch/catch.hpp"

using di = apf::discard_iterator;

TEST_CASE("iterators/discard_iterator"
    , "Test all functions of discard_iterator")
{

SECTION("everything", "")
{
  auto iter = di();
  ++iter;
  iter++;
  *iter = 42;
  *iter = 3.14;
  *iter += 1;

  // No actual checks are possible ...
}

} // TEST_CASE
