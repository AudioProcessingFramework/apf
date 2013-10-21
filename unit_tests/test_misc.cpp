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

// Tests for misc.h.

#include "apf/misc.h"

#include "catch/catch.hpp"

TEST_CASE("BlockParameter", "")
{

SECTION("default ctor", "")
{
  apf::BlockParameter<int> bp;
  CHECK(0 == bp.get());
  CHECK(0 == bp.get_old());
}

SECTION("int", "")
{
  apf::BlockParameter<int> bp(111);
  CHECK(111 == bp.get());
  CHECK(111 == bp.get_old());
  CHECK_FALSE(bp.changed());
  bp = 222;
  CHECK(222 == bp.get());
  CHECK(111 == bp.get_old());
  CHECK(bp.changed());
  bp = 333;
  CHECK(333 == bp.get());
  CHECK(222 == bp.get_old());
  CHECK(bp.changed());
}

SECTION("conversion operator", "")
{
  apf::BlockParameter<int> bp(42);
  int i = 0;
  CHECK(0 == i);
  i = bp;
  CHECK(42 == i);
  CHECK((i - bp) == 0);
}

SECTION("conversion operator from const object", "")
{
  const apf::BlockParameter<int> bp(42);
  int i = 0;
  CHECK(0 == i);
  i = bp;
  CHECK(42 == i);
  CHECK((i - bp) == 0);
}

} // TEST_CASE

// Settings for Vim (http://www.vim.org/), please do not remove:
// vim:softtabstop=2:shiftwidth=2:expandtab:textwidth=80:cindent
