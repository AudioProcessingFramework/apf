/******************************************************************************
 * Copyright © 2012-2013 Institut für Nachrichtentechnik, Universität Rostock *
 * Copyright © 2006-2012 Quality & Usability Lab,                             *
 *                       Telekom Innovation Laboratories, TU Berlin           *
 *                                                                            *
 * This file is part of the Audio Processing Framework (APF).                 *
 *                                                                            *
 * The APF is free software:  you can redistribute it and/or modify it  under *
 * the terms of the  GNU  General  Public  License  as published by the  Free *
 * Software Foundation, either version 3 of the License,  or (at your option) *
 * any later version.                                                         *
 *                                                                            *
 * The APF is distributed in the hope that it will be useful, but WITHOUT ANY *
 * WARRANTY;  without even the implied warranty of MERCHANTABILITY or FITNESS *
 * FOR A PARTICULAR PURPOSE.                                                  *
 * See the GNU General Public License for more details.                       *
 *                                                                            *
 * You should  have received a copy  of the GNU General Public License  along *
 * with this program.  If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                            *
 *                                 http://AudioProcessingFramework.github.com *
 ******************************************************************************/

// Tests for circular_iterator.

#include "apf/iterator.h"  // for circular_iterator
#include "iterator_test_macros.h"
#include "catch/catch.hpp"

typedef apf::circular_iterator<int*> ci;

TEST_CASE("iterators/circular_iterator/1"
    , "Test all straightforward functions of circular_iterator")
{

ITERATOR_TEST_SECTION_BASE(ci, int)
ITERATOR_TEST_SECTION_DEFAULT_CTOR(ci)
ITERATOR_TEST_SECTION_COPY_ASSIGNMENT(ci, int)
ITERATOR_TEST_SECTION_DEREFERENCE(ci, int, 5)
ITERATOR_TEST_SECTION_EQUALITY(ci, int)

// NOTE: comparison operators (except == and !=) don't make sense!

} // TEST_CASE

TEST_CASE("iterators/circular_iterator/2"
    , "Test all non-trivial functions of circular_iterator")
{

int a[] = { 0, 1, 2 };
ci iter1(&a[0], &a[3]);
ci iter2(&a[0], &a[3], &a[1]);

SECTION("special constructors", "")
{
  CHECK(iter1.base() == &a[0]);
  CHECK(iter2.base() == &a[1]);

  // expected errors:
  //ci iter3(&a[0]);  // not enough arguments
  //ci iter3(&a[0], &a[0]);  // assertion, begin == end
  //ci iter3(&a[0], &a[2], &a[2]);  // assertion, current == end
}

SECTION("increment", "++a; a++")
{
  CHECK(iter1.base() == &a[0]);
  iter2 = ++iter1;
  CHECK(iter1.base() == &a[1]);
  CHECK(iter2.base() == &a[1]);
  iter2 = ++iter1;
  CHECK(iter1.base() == &a[2]);
  CHECK(iter2.base() == &a[2]);
  iter2 = ++iter1;
  CHECK(iter1.base() == &a[0]);
  CHECK(iter2.base() == &a[0]);
  iter2 = ++iter1;
  CHECK(iter1.base() == &a[1]);
  CHECK(iter2.base() == &a[1]);
  iter2 = ++iter1;
  CHECK(iter1.base() == &a[2]);
  CHECK(iter2.base() == &a[2]);
  iter2 = ++iter1;
  CHECK(iter1.base() == &a[0]);
  CHECK(iter2.base() == &a[0]);

  iter2 = iter1++;
  CHECK(iter1.base() == &a[1]);
  CHECK(iter2.base() == &a[0]);
  iter2 = iter1++;
  CHECK(iter1.base() == &a[2]);
  CHECK(iter2.base() == &a[1]);
  iter2 = iter1++;
  CHECK(iter1.base() == &a[0]);
  CHECK(iter2.base() == &a[2]);
  iter2 = iter1++;
  CHECK(iter1.base() == &a[1]);
  CHECK(iter2.base() == &a[0]);
  iter2 = iter1++;
  CHECK(iter1.base() == &a[2]);
  CHECK(iter2.base() == &a[1]);
  iter2 = iter1++;
  CHECK(iter1.base() == &a[0]);
  CHECK(iter2.base() == &a[2]);
}

SECTION("decrement", "--a; a--")
{
  CHECK(iter1.base() == &a[0]);
  iter2 = --iter1;
  CHECK(iter1.base() == &a[2]);
  CHECK(iter2.base() == &a[2]);
  iter2 = --iter1;
  CHECK(iter1.base() == &a[1]);
  CHECK(iter2.base() == &a[1]);
  iter2 = --iter1;
  CHECK(iter1.base() == &a[0]);
  CHECK(iter2.base() == &a[0]);
  iter2 = --iter1;
  CHECK(iter1.base() == &a[2]);
  CHECK(iter2.base() == &a[2]);
  iter2 = --iter1;
  CHECK(iter1.base() == &a[1]);
  CHECK(iter2.base() == &a[1]);
  iter2 = --iter1;
  CHECK(iter1.base() == &a[0]);
  CHECK(iter2.base() == &a[0]);

  iter2 = iter1--;
  CHECK(iter1.base() == &a[2]);
  CHECK(iter2.base() == &a[0]);
  iter2 = iter1--;
  CHECK(iter1.base() == &a[1]);
  CHECK(iter2.base() == &a[2]);
  iter2 = iter1--;
  CHECK(iter1.base() == &a[0]);
  CHECK(iter2.base() == &a[1]);
  iter2 = iter1--;
  CHECK(iter1.base() == &a[2]);
  CHECK(iter2.base() == &a[0]);
  iter2 = iter1--;
  CHECK(iter1.base() == &a[1]);
  CHECK(iter2.base() == &a[2]);
  iter2 = iter1--;
  CHECK(iter1.base() == &a[0]);
  CHECK(iter2.base() == &a[1]);
}

SECTION("plus/minus", "a + n; n + a; a - n; a - b; a += n; a -= n")
{
  CHECK((iter1 + -9) == &a[0]);
  CHECK((iter1 + -8) == &a[1]);
  CHECK((iter1 + -7) == &a[2]);
  CHECK((iter1 + -6) == &a[0]);
  CHECK((iter1 + -5) == &a[1]);
  CHECK((iter1 + -4) == &a[2]);
  CHECK((iter1 + -3) == &a[0]);
  CHECK((iter1 + -2) == &a[1]);
  CHECK((iter1 + -1) == &a[2]);
  CHECK((iter1 +  0) == &a[0]);
  CHECK((iter1 +  1) == &a[1]);
  CHECK((iter1 +  2) == &a[2]);
  CHECK((iter1 +  3) == &a[0]);
  CHECK((iter1 +  4) == &a[1]);
  CHECK((iter1 +  5) == &a[2]);
  CHECK((iter1 +  6) == &a[0]);
  CHECK((iter1 +  7) == &a[1]);
  CHECK((iter1 +  8) == &a[2]);
  CHECK((iter1 +  9) == &a[0]);

  CHECK((iter1 -  9) == &a[0]);
  CHECK((iter1 -  8) == &a[1]);
  CHECK((iter1 -  7) == &a[2]);
  CHECK((iter1 -  6) == &a[0]);
  CHECK((iter1 -  5) == &a[1]);
  CHECK((iter1 -  4) == &a[2]);
  CHECK((iter1 -  3) == &a[0]);
  CHECK((iter1 -  2) == &a[1]);
  CHECK((iter1 -  1) == &a[2]);
  CHECK((iter1 -  0) == &a[0]);
  CHECK((iter1 - -1) == &a[1]);
  CHECK((iter1 - -2) == &a[2]);
  CHECK((iter1 - -3) == &a[0]);
  CHECK((iter1 - -4) == &a[1]);
  CHECK((iter1 - -5) == &a[2]);
  CHECK((iter1 - -6) == &a[0]);
  CHECK((iter1 - -7) == &a[1]);
  CHECK((iter1 - -8) == &a[2]);
  CHECK((iter1 - -9) == &a[0]);

  CHECK((0 + iter1) == &a[0]);
  CHECK((1 + iter1) == &a[1]);
  CHECK((2 + iter1) == &a[2]);
  CHECK((3 + iter1) == &a[0]);
  CHECK((4 + iter1) == &a[1]);
  CHECK((5 + iter1) == &a[2]);
  CHECK((6 + iter1) == &a[0]);
  CHECK((7 + iter1) == &a[1]);
  CHECK((8 + iter1) == &a[2]);
  CHECK((9 + iter1) == &a[0]);

  CHECK((ci(&a[0], &a[3], &a[0]) - ci(&a[0], &a[3])) == 0);
  CHECK((ci(&a[0], &a[3], &a[1]) - ci(&a[0], &a[3])) == 1);
  CHECK((ci(&a[0], &a[3], &a[2]) - ci(&a[0], &a[3])) == 2);

  // all differences are positive!
  CHECK((ci(&a[0], &a[3]) - ci(&a[0], &a[3], &a[0])) == 0);
  CHECK((ci(&a[0], &a[3]) - ci(&a[0], &a[3], &a[1])) == 2);
  CHECK((ci(&a[0], &a[3]) - ci(&a[0], &a[3], &a[2])) == 1);

  iter2 = (iter1 += 0);
  CHECK(iter1.base() == &a[0]);
  CHECK(iter2.base() == &a[0]);
  iter1 += 2;
  CHECK(iter1.base() == &a[2]);
  iter1 += 2;
  CHECK(iter1.base() == &a[1]);
  iter1 -= 2;
  CHECK(iter1.base() == &a[2]);
  iter1 -= 2;
  CHECK(iter1.base() == &a[0]);

  ci iter3(&a[0]);  // "useless" constructor
  CHECK((iter3 + 666) == &a[0]);
}

SECTION("offset dereference", "a[n]")
{
  CHECK(iter1[-5] == 1);
  CHECK(iter1[-4] == 2);
  CHECK(iter1[-3] == 0);
  CHECK(iter1[-2] == 1);
  CHECK(iter1[-1] == 2);
  CHECK(iter1[ 0] == 0);
  CHECK(iter1[ 1] == 1);
  CHECK(iter1[ 2] == 2);
  CHECK(iter1[ 3] == 0);
  CHECK(iter1[ 4] == 1);
  CHECK(iter1[ 5] == 2);

  // can we also assign?
  iter1[-3] = 42;
  CHECK(a[0] == 42);
}

} // TEST_CASE

#include <list>

TEST_CASE("iterators/circular_iterator/3"
    , "Test if it also works with a bidirectional iterator")
{

  std::list<int> l;
  l.push_back(0);
  l.push_back(1);
  l.push_back(2);
  apf::circular_iterator<std::list<int>::iterator> it(l.begin(), l.end());

  CHECK(*it == 0);
  --it;
  CHECK(*it == 2);
  it--;
  CHECK(*it == 1);
  ++it;
  CHECK(*it == 2);
  it++;
  CHECK(*it == 0);

} // TEST_CASE

// Settings for Vim (http://www.vim.org/), please do not remove:
// vim:softtabstop=2:shiftwidth=2:expandtab:textwidth=80:cindent
