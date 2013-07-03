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

// Tests for transform_iterator.

#include "apf/iterator.h"  // for transform_iterator
#include "iterator_test_macros.h"

#include "catch/catch.hpp"

// Requirements for input iterators:
// http://www.cplusplus.com/reference/std/iterator/InputIterator/

struct three_halves
{
  typedef int argument_type;  // this is optional
  typedef float result_type;  // this is necessary

  float operator()(int in) { return static_cast<float>(in) * 1.5f; }
};

typedef apf::transform_iterator<int*, three_halves> fii;

TEST_CASE("iterators/transform_iterator"
    , "Test all functions of transform_iterator")
{

ITERATOR_TEST_SECTION_BASE(fii, int)
ITERATOR_TEST_SECTION_DEFAULT_CTOR(fii)
ITERATOR_TEST_SECTION_COPY_ASSIGNMENT(fii, int)
ITERATOR_TEST_SECTION_EQUALITY(fii, int)
ITERATOR_TEST_SECTION_INCREMENT(fii, int)
ITERATOR_TEST_SECTION_DECREMENT(fii, int)
ITERATOR_TEST_SECTION_PLUS_MINUS(fii, int)
ITERATOR_TEST_SECTION_LESS(fii, int)

int array[] = { 1, 2, 3 };

SECTION("dereference", "*a; *a++; a[]")
{
  fii iter(array);

  CHECK(*iter == 1.5f);
  CHECK(iter.base() == &array[0]);
  CHECK(*iter++ == 1.5f);
  CHECK(iter.base() == &array[1]);

  CHECK(*iter-- == 3.0f);
  CHECK(iter.base() == &array[0]);

  CHECK(iter[2] == 4.5f);

  // NOTE: operator->() doesn't make too much sense here, it's not working.
  // See below for an example where it does work.
}

SECTION("test make_transform_iterator", "namespace-level helper function")
{
  CHECK(*apf::make_transform_iterator(array, three_halves()) == 1.5f);
}

} // TEST_CASE

struct mystruct
{
  struct inner
  {
    inner() : num(42) {}
    int num;
  } innerobj;
};

struct special_function
{
  typedef mystruct::inner& result_type;

  mystruct::inner& operator()(mystruct& s)
  {
    return s.innerobj;
  }
};

TEST_CASE("iterators/transform_iterator with strange function"
    , "Test transform_iterator with a special function object")
{

SECTION("only one section", "test special_function")
{
  mystruct x;
  apf::transform_iterator<mystruct*, special_function>
    it(&x, special_function());
  CHECK(&*it == &x.innerobj);
  CHECK(it->num == 42);
}

} // TEST_CASE

TEST_CASE("transform_proxy", "Test transform_proxy")
{

typedef std::vector<int> vi;
vi input;
input.push_back(1);
input.push_back(2);
input.push_back(3);

SECTION("test transform_proxy", "")
{
  apf::transform_proxy<three_halves, vi> p(input);

  CHECK(p.size() == 3);
  CHECK((*p.begin()) == 1.5);
  CHECK((p.begin() + 3) == p.end());
}

SECTION("test tranform_proxy_const", "")
{
  apf::transform_proxy_const<three_halves, vi> p(input);

  CHECK(p.size() == 3);
  CHECK((*p.begin()) == 1.5);
  CHECK((p.begin() + 3) == p.end());
}

} // TEST_CASE

// Settings for Vim (http://www.vim.org/), please do not remove:
// vim:softtabstop=2:shiftwidth=2:expandtab:textwidth=80:cindent
