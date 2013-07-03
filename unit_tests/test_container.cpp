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

// Tests for fixed_vector, fixed_list, fixed_matrix

#include "apf/container.h"

#include "catch/catch.hpp"

class NonCopyable
{
  public:
    NonCopyable() {}

  private:
    NonCopyable(const NonCopyable&);
    NonCopyable& operator=(const NonCopyable&);
};

TEST_CASE("fixed_vector", "Test fixed_vector")
{

SECTION("default constructor", "")
{
  apf::fixed_vector<int> fv;
  CHECK(fv.size() == 0);
}

SECTION("first constructor", "")
{
  apf::fixed_vector<int> fv(3);
  CHECK(fv[1] == 0);
}

SECTION("second constructor", "")
{
  apf::fixed_vector<int> fv(3ul, 99);

  CHECK(fv[2] == 99);
}

SECTION("another constructor", "")
{
  apf::fixed_vector<int> fv(std::make_pair(3ul, 99));

  CHECK(fv[2] == 99);
}

SECTION("yet another constructor and more", "")
{
  int data[] = { 1, 2, 3, 4 };
  apf::fixed_vector<int> fv(data, data+4);
  CHECK(fv[1] == 2);
  fv[1] = 100;
  CHECK(fv[1] == 100);

  CHECK(*fv.begin() == 1);

  const apf::fixed_vector<int> cfv(data, data+4);

  CHECK(*cfv.begin() == 1);
  CHECK(cfv[2] == 3);

  CHECK(*fv.rbegin() == 4);
  CHECK(*cfv.rbegin() == 4);

  CHECK(fv.size() == 4);
  CHECK_FALSE(fv.empty());

  CHECK(fv.front() == 1);
  CHECK(fv.back() == 4);
}

SECTION("empty()", "not really useful ...")
{
  apf::fixed_vector<int> fv(0);
  CHECK(fv.empty());
}

SECTION("reserve(), emplace_back()", "")
{
  apf::fixed_vector<int> fv;
  CHECK(fv.empty());
  fv.reserve(2);
  CHECK(fv.empty());
  fv.emplace_back(1);
  CHECK(fv.size() == 1);
  CHECK(fv[0] == 1);
  fv.emplace_back(2);
  CHECK(fv[1] == 2);
  fv.reserve(0);
  CHECK(fv.empty());
  CHECK_THROWS_AS(fv.emplace_back(666), std::out_of_range);
}

SECTION("reset()", "")
{
  apf::fixed_vector<int> fv(3);
  CHECK(fv.size() == 3);
  fv.reset(5);
  CHECK(fv.size() == 5);
  fv.reset(0);
  CHECK(fv.size() == 0);
  fv.reset(2ul, 4);
  CHECK(fv.size() == 2);
  CHECK(fv[0] == 4);
  CHECK(fv[1] == 4);

  int data[] = { 1, 2, 3, 4 };
  fv.reset(data, data + 4);
  CHECK(fv.size() == 4);
  CHECK(fv[0] == 1);
  CHECK(fv[1] == 2);
  CHECK(fv[2] == 3);
  CHECK(fv[3] == 4);
}

SECTION("fixed_vector<NonCopyable>", "")
{
  apf::fixed_vector<NonCopyable> fv(1000);

  //std::vector<NonCopyable> v(1000);  // this wouldn't work!
}

} // TEST_CASE fixed_vector

TEST_CASE("fixed_list", "Test fixed_list")
{

SECTION("first constructor", "")
{
  apf::fixed_list<int> fl(3);
  CHECK(fl.size() == 3);
  CHECK(fl.front() == 0);
}

SECTION("second constructor", "")
{
  apf::fixed_list<int> fl(std::make_pair(3ul, 99));
  CHECK(fl.size() == 3);
  CHECK(fl.front() == 99);
  CHECK(fl.back() == 99);
}

SECTION("third constructor and more", "")
{
  int data[] = { 1, 2, 3, 4 };
  apf::fixed_list<int> fl(data, data+4);
  CHECK(*fl.begin() == 1);
  CHECK(*(--fl.end()) == 4);
  CHECK(*fl.rbegin() == 4);
  CHECK(*(--fl.rend()) == 1);

  CHECK(fl.front() == 1);
  CHECK(fl.back() == 4);
  fl.front() = 100;
  CHECK(fl.front() == 100);
  fl.front() = 1;

  CHECK(fl.size() == 4);
  CHECK_FALSE(fl.empty());

  fl.move(fl.begin(), fl.end());
  CHECK(*(fl.begin()) == 2);
  CHECK(*(++fl.begin()) == 3);
  CHECK(*(++++fl.begin()) == 4);
  CHECK(*(++++++fl.begin()) == 1);

  fl.move(++fl.begin(), fl.end());
  CHECK(*(fl.begin()) == 2);
  CHECK(*(++fl.begin()) == 4);
  CHECK(*(++++fl.begin()) == 1);
  CHECK(*(++++++fl.begin()) == 3);

  fl.move(--fl.end(), fl.begin());
  CHECK(*(fl.begin()) == 3);
  CHECK(*(++fl.begin()) == 2);
  CHECK(*(++++fl.begin()) == 4);
  CHECK(*(++++++fl.begin()) == 1);

  fl.move(++fl.begin(), ++++++fl.begin(), fl.begin());
  CHECK(*(fl.begin()) == 2);
  CHECK(*(++fl.begin()) == 4);
  CHECK(*(++++fl.begin()) == 3);
  CHECK(*(++++++fl.begin()) == 1);

  const apf::fixed_list<int> cfl(data, data+4);

  CHECK(cfl.front() == 1);
  CHECK(cfl.back() == 4);

  CHECK(*cfl.begin() == 1);
  CHECK(*(--cfl.end()) == 4);
  CHECK(*cfl.rbegin() == 4);
  CHECK(*(--cfl.rend()) == 1);

  CHECK(cfl.size() == 4);
  CHECK_FALSE(cfl.empty());
}

SECTION("empty()", "not really useful ...")
{
  apf::fixed_list<int> fl(0);
  CHECK(fl.empty());
}

SECTION("fixed_list<NonCopyable>", "")
{
  apf::fixed_list<NonCopyable> fl(1000);

  //std::list<NonCopyable> v(1000);  // this wouldn't work!
}

} // TEST_CASE fixed_list

typedef apf::fixed_matrix<int> fm;

TEST_CASE("fixed_matrix", "Test fixed_matrix")
{

SECTION("default constructor", "... and reset()")
{
  fm matrix;
  CHECK(matrix.empty());
  CHECK(matrix.channels.begin() == matrix.channels.end());
  CHECK(matrix.slices.begin() == matrix.slices.end());

  matrix.reset(2, 3);
  CHECK_FALSE(matrix.empty());
  CHECK(std::distance(matrix.channels.begin(), matrix.channels.end()) == 2);
  CHECK(std::distance(matrix.slices.begin(), matrix.slices.end()) == 3);
}

SECTION("the normal constructor and more", "")
{
  fm matrix(3, 2);
  CHECK_FALSE(matrix.empty());
  CHECK(std::distance(matrix.channels.begin(), matrix.channels.end()) == 3);
  CHECK(std::distance(matrix.slices.begin(), matrix.slices.end()) == 2);

  matrix.channels[2][0] = 42;

  fm matrix2(2, 3);
  matrix2.set_channels(matrix.slices);

  CHECK(matrix2.channels[0][2] == 42);
  CHECK(matrix2.slices[2][0] == 42);

  CHECK(matrix2.get_channel_ptrs()[0][2] == 42);
}

// TODO: check channels_iterator and slices_iterator

} // TEST_CASE fixed_matrix

#include <list>

struct ClassWithSublist
{
  std::list<int> sublist;
};

TEST_CASE("misc", "the rest")
{

SECTION("append_pointers()", "")
{
  apf::fixed_vector<int> v(3);
  std::list<int*> target;
  apf::append_pointers(v, target);
  CHECK(*target.begin() == &*v.begin());
}

SECTION("const append_pointers()", "")
{
  const apf::fixed_vector<int> v(3);
  std::list<const int*> target;
  apf::append_pointers(v, target);
  CHECK(*target.begin() == &*v.begin());
}

SECTION("distribute_list()", "... and undistribute_list()")
{
  std::list<int> in;
  in.push_back(1);
  in.push_back(2);
  in.push_back(3);
  apf::fixed_vector<ClassWithSublist> out(3);

  distribute_list(in, out, &ClassWithSublist::sublist);

  CHECK(in.empty() == true);
  // lists have different size -> exception:
  CHECK_THROWS_AS(distribute_list(in, out, &ClassWithSublist::sublist)
      , std::logic_error);
  CHECK(out[2].sublist.size() == 1);
  CHECK(out[2].sublist.front() == 3);

  in.clear();
  in.push_back(4);
  in.push_back(5);
  in.push_back(6);

  distribute_list(in, out, &ClassWithSublist::sublist);
  CHECK(out[2].sublist.size() == 2);
  CHECK(out[2].sublist.front() == 3);
  CHECK(out[2].sublist.back() == 6);

  CHECK(in.size() == 0);  // 'in' is empty again ...

  // For undistribute_list(), the first argument can be a different type:
  apf::fixed_vector<int> in2(3);
  in2[0] = 1;
  in2[1] = 2;
  in2[2] = 3;

  std::list<int> garbage;

  undistribute_list(in2, out, &ClassWithSublist::sublist, garbage);

  CHECK(garbage.size() == 3);
  CHECK(in2.size() == 3);
  CHECK(out[2].sublist.size() == 1);
  CHECK(out[2].sublist.front() == 6);

  in.push_back(666);

  // in and out have different size -> exception:
  CHECK_THROWS_AS(undistribute_list(in, out, &ClassWithSublist::sublist
        , garbage), std::logic_error);

  CHECK(in.size() == 1);

  in.push_back(5);
  in.push_back(6);

  // list item is not found -> exception:
  CHECK_THROWS_AS(undistribute_list(in, out, &ClassWithSublist::sublist
        , garbage), std::logic_error);

  CHECK(in.size() == 3);

  in.front() = 4;

  CHECK_NOTHROW(undistribute_list(in, out, &ClassWithSublist::sublist,garbage));
}

} // TEST_CASE misc

// Settings for Vim (http://www.vim.org/), please do not remove:
// vim:softtabstop=2:shiftwidth=2:expandtab:textwidth=80:cindent
