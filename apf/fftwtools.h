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

/// @file
/// Some tools for the use with the FFTW library.

#ifndef APF_FFTWTOOLS_H
#define APF_FFTWTOOLS_H

#include <fftw3.h>

#include <limits>  // for std::numeric_limits

namespace apf
{

/// Traits class to select float/double/long double versions of FFTW functions
/// @see fftw<float>, fftw<double>, fftw<long double>, APF_FFTW_TRAITS
/// @note This is by far not complete, but it's trivial to extend.
template<typename T> struct fftw {};  // Most general case is empty, see below!

/// Macro to create traits classes for float/double/long double
#define APF_FFTW_TRAITS(longtype, shorttype) \
/** <b>longtype</b> specialization of the traits class @ref fftw **/ \
/** @see APF_FFTW_TRAITS **/ \
template<> \
struct fftw<longtype> { \
  typedef fftw ## shorttype ## plan plan; \
  static void* malloc(size_t n) { return fftw ## shorttype ## malloc(n); } \
  static void free(void* p) { fftw ## shorttype ## free(p); } \
  static void destroy_plan(plan p) { \
    fftw ## shorttype ## destroy_plan(p); p = 0; } \
  static void execute(const plan p) { fftw ## shorttype ## execute(p); } \
  static void execute_r2r(const plan p, longtype *in, longtype *out) { \
    fftw ## shorttype ## execute_r2r(p, in, out); } \
  static plan plan_r2r_1d(int n, longtype* in, longtype* out \
      , fftw_r2r_kind kind, unsigned flags) { \
    return fftw ## shorttype ## plan_r2r_1d(n, in, out, kind, flags); } \
};

APF_FFTW_TRAITS(float, f_)
APF_FFTW_TRAITS(double, _)
APF_FFTW_TRAITS(long double, l_)

/// @note: This only works for containers with contiguous memory (e.g. vector)!
template<typename T>
struct fftw_allocator
{
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  typedef T value_type;

  pointer allocate(size_type n, const void* hint = 0)
  {
    (void)hint;
    return static_cast<pointer>(fftw<T>::malloc(sizeof(value_type) * n));
  }

  void deallocate(pointer p, size_type n)
  {
    (void)n;
    fftw<T>::free(p);
  }

  void construct(pointer p, const T& t) { new (p) T(t); }
  void destroy(pointer p) { p->~T(); }

  size_type max_size() const
  {
    return std::numeric_limits<size_type>::max() / sizeof(T);
  }

  template<typename U>
  struct rebind { typedef fftw_allocator<U> other; };

  // Not sure if the following are necessary ...

  fftw_allocator() {}
  fftw_allocator(const fftw_allocator&) {}
  template<typename U> fftw_allocator(const fftw_allocator<U>&) {}

  pointer address(reference value) { return &value; }
  const_pointer address(const_reference value) { return &value; }

  template<typename U>
  bool operator==(const fftw_allocator<U>&) { return true; }

  template<typename U>
  bool operator!=(const fftw_allocator<U>&) { return false; }
};

}  // namespace apf

#endif

// Settings for Vim (http://www.vim.org/), please do not remove:
// vim:softtabstop=2:shiftwidth=2:expandtab:textwidth=80:cindent
// vim:fdm=expr:foldexpr=getline(v\:lnum)=~'/\\*\\*'&&getline(v\:lnum)!~'\\*\\*/'?'a1'\:getline(v\:lnum)=~'\\*\\*/'&&getline(v\:lnum)!~'/\\*\\*'?'s1'\:'='
