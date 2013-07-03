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

// Tests for stride_iterator.

#define APF_STRIDE_ITERATOR_DEFAULT_STRIDE 1
#include "apf/iterator.h"  // for stride_iterator
#include "iterator_test_macros.h"

#include "catch/catch.hpp"

typedef apf::stride_iterator<int*> si;

TEST_CASE("iterators/stride_iterator", "Test all functions of stride_iterator")
{

// First, the straightforward functions (with a default stride of 1)

ITERATOR_TEST_SECTION_BASE(si, int)
ITERATOR_TEST_SECTION_DEFAULT_CTOR(si)
ITERATOR_TEST_SECTION_COPY_ASSIGNMENT(si, int)
ITERATOR_TEST_SECTION_DEREFERENCE(si, int, 5)
ITERATOR_TEST_SECTION_OFFSET_DEREFERENCE(si, int, 5, 6)
ITERATOR_TEST_SECTION_EQUALITY(si, int)
ITERATOR_TEST_SECTION_INCREMENT(si, int)
ITERATOR_TEST_SECTION_DECREMENT(si, int)
ITERATOR_TEST_SECTION_PLUS_MINUS(si, int)
ITERATOR_TEST_SECTION_LESS(si, int)

// ... then the specific stride_iterator stuff

SECTION("stride", "Test if stride works.")
{
  int array[9];

  si iter(array, 2);

  CHECK(iter.base() == &array[0]);
  CHECK(iter.step_size() == 2);

  ++iter;
  CHECK(iter.base() == &array[2]);

  iter++;
  CHECK(iter.base() == &array[4]);

  CHECK((iter + 2).base() == &array[8]);
  CHECK((2 + iter).base() == &array[8]);

  iter += 2;
  CHECK(iter.base() == &array[8]);

  iter--;
  CHECK(iter.base() == &array[6]);

  --iter;
  CHECK(iter.base() == &array[4]);

  CHECK((iter - 2).base() == &array[0]);

  iter -= 2;
  CHECK(iter.base() == &array[0]);
}

SECTION("special constructor"
    , "Test if constructor from another stride_iterator works.")
{
  int array[9];

  si iter1(array, 2);
  CHECK(iter1.step_size() == 2);

  si iter2(iter1, 3);
  CHECK(iter2.step_size() == 6);

  ++iter2;

  CHECK(iter2.base() == &array[6]);
}

} // TEST_CASE

// Settings for Vim (http://www.vim.org/), please do not remove:
// vim:softtabstop=2:shiftwidth=2:expandtab:textwidth=80:cindent
