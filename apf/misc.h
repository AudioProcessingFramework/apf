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
/// Miscellaneous helper classes.

#ifndef APF_MISC_H
#define APF_MISC_H

#include <utility>  // for std::forward

namespace apf
{

/// Classes derived from this class cannot be copied.
/// Private copy constructor and copy assignment ensure that.
class NonCopyable
{
  protected:
     NonCopyable() {} ///< Protected default constructor
    ~NonCopyable() {} ///< Protected non-virtual destructor

  private:
    /// @name Deactivated copy ctor and assignment operator
    //@{
    NonCopyable(const NonCopyable&);
    NonCopyable& operator=(const NonCopyable&);
    //@}
};

template<typename T>
class BlockParameter
{
  public:
    template<typename... Args>
    explicit BlockParameter(Args&&... args)
      : _current{args...}
      , _old{std::forward<Args>(args)...}
    {}

    template<typename Arg>
    T& operator=(Arg&& arg)
    {
      _old = std::move(_current);
      return _current = std::forward<Arg>(arg);
    }

    const T& get() const { return _current; }
    const T& get_old() const { return _old; }

    operator const T&() const { return this->get(); }

    bool changed() const { return this->get() != this->get_old(); }

  private:
    T _current, _old;
};

} // namespace apf

#endif

// Settings for Vim (http://www.vim.org/), please do not remove:
// vim:softtabstop=2:shiftwidth=2:expandtab:textwidth=80:cindent
