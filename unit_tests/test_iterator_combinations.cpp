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

// Tests for combinations of different iterators.

#include "apf/iterator.h"  // for *_iterator

#include "catch/catch.hpp"

struct three_halves
{
  typedef int argument_type;
  typedef float result_type;

  float operator()(int in) { return static_cast<float>(in) * 1.5f; }
};

typedef apf::index_iterator<int> ii;
typedef apf::transform_iterator<ii, three_halves> fii;
typedef apf::stride_iterator<ii> si;
typedef apf::transform_iterator<si, three_halves> fsi;

TEST_CASE("iterators/combinations", "Test combinations of iterators")
{

SECTION("index_iterator + transform_iterator", "")
{
  fii iter(apf::make_index_iterator(2), three_halves());
  CHECK(*iter == 3.0f);
}

SECTION("index_iterator + stride_iterator + transform_iterator", "")
{
  fsi iter(si(apf::make_index_iterator(2), 2), three_halves());
  CHECK(*iter == 3.0f);
  ++iter;
  CHECK(*iter == 6.0f);

  fsi iter2(si(apf::make_index_iterator(2), -2), three_halves());
  CHECK(*iter2 == 3.0f);
  ++iter2;
  CHECK(*iter2 == 0.0f);
}


} // TEST_CASE

// Settings for Vim (http://www.vim.org/), please do not remove:
// vim:softtabstop=2:shiftwidth=2:expandtab:textwidth=80:cindent
