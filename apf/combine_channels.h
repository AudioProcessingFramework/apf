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
/// Combine channels, interpolate, crossfade.

#ifndef APF_COMBINE_CHANNELS_H
#define APF_COMBINE_CHANNELS_H

#include <vector>
#include <cassert>  // for assert()
#include <stdexcept>  // for std::logic_error
#include <algorithm>  // for std::transform(), std::copy(), std::fill()

#include <tr1/functional>  // for std::tr1::bind()
#include <tr1/type_traits>  // for std::tr1::remove_reference

#include "apf/iterator.h" // for *_iterator, make_*_iterator(), cast_proxy_const

namespace apf
{

/** Base class for CombineChannels*.
 * @tparam Derived Derived class ("Curiously Recurring Template Pattern")
 * @tparam ListProxy Proxy class for input list. If no proxy is needed, just use
 *   a reference to the list (e.g. std::list<myitem>&).
 *   @p ListProxy (or the list itself) must have begin() and end() and an inner
 *   type @c value_type which itself must have begin() and end() and an inner
 *   type @c iterator.
 * @tparam Out Output class. Must have begin() and end() functions.
 *
 * @see CombineChannels, CombineChannelsCopy, CombineChannelsCrossfade,
 *   CombineChannelsCrossfadeCopy, CombineChannelsInterpolation
 **/
template<typename Derived, typename ListProxy, typename Out>
class CombineChannelsBase
{
  protected:
    typedef typename std::iterator_traits<typename std::tr1::remove_reference<
      ListProxy>::type::value_type::iterator>::value_type T;

  public:
    /// Constructor.
    /// @param in List of objects to combine
    /// @param out Target object
    template<typename L>
    CombineChannelsBase(L& in, Out& out)
      : _in(in)
      , _out(out)
    {}

    /// Do the actual combining.
    template<typename F>
    void process(F f)
    {
      _accumulate = false;

      static_cast<Derived*>(this)->before_the_loop();

      for (typename std::tr1::remove_reference<ListProxy>::type::iterator item
          = _in.begin()
          ; item != _in.end()
          ; ++item)
      {
        switch (f.select(*item))
        {
          case 0:
            // Do nothing
            continue;  // jump to next list item

          case 1:
            static_cast<Derived*>(this)->case_one(*item, f);
            break;

          case 2:
            static_cast<Derived*>(this)->case_two(*item, f);
            break;

          default:
            throw std::runtime_error("Predicate must return 0, 1 or 2!");
        }
      }

      static_cast<Derived*>(this)->after_the_loop();

      if (!_accumulate)
      {
        std::fill(_out.begin(), _out.end(), T());
      }
    }

    void before_the_loop() {}

    template<typename ItemType, typename F>
    void case_one(const ItemType&, F)
    {
      throw std::logic_error("CombineChannelsBase: case 1 not implemented!");
    }

    template<typename ItemType, typename F>
    void case_two(const ItemType&, F)
    {
      throw std::logic_error("CombineChannelsBase: case 2 not implemented!");
    }

    void after_the_loop() {}

  private:
    ListProxy _in;

  protected:
    template<typename ItemType>
    void _case_one_copy(const ItemType& item)
    {
      if (_accumulate)
      {
        std::copy(item.begin(), item.end()
            , make_accumulating_iterator(_out.begin()));
      }
      else
      {
        std::copy(item.begin(), item.end(), _out.begin());
        _accumulate = true;
      }
    }

    template<typename ItemType, typename FunctionType>
    void _case_one_transform(const ItemType& item, FunctionType f)
    {
      if (_accumulate)
      {
        std::transform(item.begin(), item.end()
            , make_accumulating_iterator(_out.begin()), f);
      }
      else
      {
        std::transform(item.begin(), item.end(), _out.begin(), f);
        _accumulate = true;
      }
    }

    Out& _out;
    bool _accumulate;
};

/** Combine channels: accumulate.
 **/
template<typename L, typename Out>
class CombineChannelsCopy : public CombineChannelsBase<
                                            CombineChannelsCopy<L, Out>, L, Out>
{
  private:
    typedef CombineChannelsBase<CombineChannelsCopy<L, Out>, L, Out> _base;

  public:
    CombineChannelsCopy(const L& in, Out& out) : _base(in, out) {}

    template<typename ItemType, typename F>
    void case_one(const ItemType& item, F)
    {
      this->_case_one_copy(item);
    }

    // Case 2 is not implemented and shall not be used!
};

/** Combine channels: transform and accumulate.
 **/
template<typename L, typename Out>
class CombineChannels: public CombineChannelsBase<
                                                CombineChannels<L, Out>, L, Out>
{
  private:
    typedef CombineChannelsBase<CombineChannels<L, Out>, L, Out> _base;

  public:
    CombineChannels(const L& in, Out& out) : _base(in, out) {}

    template<typename ItemType, typename F>
    void case_one(const ItemType& item, F f)
    {
      this->_case_one_transform(item, f);
    }

    // Case 2 is not implemented and shall not be used!
};

/** Combine channels: interpolate and accumulate.
 **/
template<typename L, typename Out>
class CombineChannelsInterpolation: public CombineChannelsBase<
                                   CombineChannelsInterpolation<L, Out>, L, Out>
{
  private:
    typedef CombineChannelsBase<CombineChannelsInterpolation<L, Out>, L, Out>
      _base;
    typedef typename _base::T T;
    using _base::_accumulate;
    using _base::_out;

  public:
    CombineChannelsInterpolation(const L& in, Out& out) : _base(in, out) {}

    template<typename ItemType, typename F>
    void case_one(const ItemType& item, F f)
    {
      this->_case_one_transform(item, f);
    }

    template<typename ItemType, typename F>
    void case_two(const ItemType& item, F f)
    {
      if (_accumulate)
      {
        std::transform(item.begin(), item.end(), index_iterator<T>()
            , make_accumulating_iterator(_out.begin()), f);
      }
      else
      {
        std::transform(item.begin(), item.end(), index_iterator<T>()
            , _out.begin(), f);
        _accumulate = true;
      }
    }
};

struct fade_out_tag {};

/** Base class for CombineChannelsCrossfade*.
 **/
template<typename Derived, typename L, typename Out, typename Crossfade>
class CombineChannelsCrossfadeBase : public CombineChannelsBase<Derived, L, Out>
{
  private:
    typedef CombineChannelsBase<Derived, L, Out> _base;
    typedef typename _base::T T;
    using _base::_accumulate;
    using _base::_out;

  public:
    CombineChannelsCrossfadeBase(const L& in, Out& out, const Crossfade& fade)
      : _base(in, out)
      , _fade_out_buffer(fade.size())
      , _fade_in_buffer(fade.size())
      , _crossfade_data(fade)
    {}

    void before_the_loop()
    {
      _accumulate_fade = false;
    }

    void after_the_loop()
    {
      if (_accumulate_fade)
      {
        if (_accumulate)
        {
          std::transform(_fade_out_buffer.begin(), _fade_out_buffer.end()
              , _crossfade_data.fade_out_begin()
              , make_accumulating_iterator(_out.begin())
              , std::multiplies<T>());
        }
        else
        {
          std::transform(_fade_out_buffer.begin(), _fade_out_buffer.end()
              , _crossfade_data.fade_out_begin(), _out.begin()
              , std::multiplies<T>());
          _accumulate = true;
        }

        // Fade-in is always accumulated:
        std::transform(_fade_in_buffer.begin(), _fade_in_buffer.end()
            , _crossfade_data.fade_in_begin()
            , make_accumulating_iterator(_out.begin())
            , std::multiplies<T>());
      }
    }

  protected:
    bool _accumulate_fade;
    std::vector<T> _fade_out_buffer, _fade_in_buffer;

  private:
    const Crossfade& _crossfade_data;
};

/** Combine channels: crossfade and accumulate.
 **/
template<typename L, typename Out, typename Crossfade>
class CombineChannelsCrossfadeCopy : public CombineChannelsCrossfadeBase<
             CombineChannelsCrossfadeCopy<L, Out, Crossfade>, L, Out, Crossfade>
{
  private:
    typedef CombineChannelsCrossfadeBase<CombineChannelsCrossfadeCopy<
      L, Out, Crossfade>, L, Out, Crossfade> _base;

    using _base::_fade_out_buffer;
    using _base::_fade_in_buffer;
    using _base::_accumulate_fade;

  public:
    CombineChannelsCrossfadeCopy(const L& in, Out& out, const Crossfade& fade)
      : _base(in, out, fade)
    {}

    template<typename ItemType, typename F>
    void case_one(const ItemType& item, F)
    {
      this->_case_one_copy(item);
    }

    template<typename ItemType, typename F>
    void case_two(ItemType& item, F)
    {
      if (_accumulate_fade)
      {
        std::copy(item.begin(), item.end()
            , make_accumulating_iterator(_fade_out_buffer.begin()));
        item.update();
        std::copy(item.begin(), item.end()
            , make_accumulating_iterator(_fade_in_buffer.begin()));
      }
      else
      {
        std::copy(item.begin(), item.end(), _fade_out_buffer.begin());
        item.update();
        std::copy(item.begin(), item.end(), _fade_in_buffer.begin());
      }
      _accumulate_fade = true;
    }
};

/** Combine channels: transform, crossfade and accumulate.
 **/
template<typename L, typename Out, typename Crossfade>
class CombineChannelsCrossfade : public CombineChannelsCrossfadeBase<
                 CombineChannelsCrossfade<L, Out, Crossfade>, L, Out, Crossfade>
{
  private:
    typedef CombineChannelsCrossfadeBase<CombineChannelsCrossfade<
      L, Out, Crossfade>, L, Out, Crossfade> _base;

  public:
    CombineChannelsCrossfade(const L& in, Out& out, const Crossfade& fade)
      : _base(in, out, fade)
    {}

    template<typename ItemType, typename F>
    void case_one(const ItemType& item, F f)
    {
      this->_case_one_transform(item, f);
    }

    template<typename ItemType, typename F>
    void case_two(ItemType& item, F f)
    {
      if (this->_accumulate_fade)
      {
        std::transform(item.begin(), item.end()
            , make_accumulating_iterator(this->_fade_out_buffer.begin())
            //, std::bind2nd(f, fade_out_tag()));
            , std::tr1::bind(f, std::tr1::placeholders::_1, fade_out_tag()));
        item.update();
        std::transform(item.begin(), item.end()
            , make_accumulating_iterator(this->_fade_in_buffer.begin()), f);
      }
      else
      {
        std::transform(item.begin(), item.end()
            , this->_fade_out_buffer.begin()
            //, std::bind2nd(f, fade_out_tag()));
            , std::tr1::bind(f, std::tr1::placeholders::_1, fade_out_tag()));
        item.update();
        std::transform(item.begin(), item.end()
            , this->_fade_in_buffer.begin(), f);
      }
      this->_accumulate_fade = true;
    }
};

/** Crossfade using a raised cosine.
 **/
template<typename T>
class raised_cosine_fade
{
  private:
    typedef transform_iterator<index_iterator<T>, math::raised_cosine<T> >
      iterator_type;

  public:
    typedef typename std::vector<T>::const_iterator iterator;
    typedef typename std::vector<T>::const_reverse_iterator reverse_iterator;

    raised_cosine_fade(size_t block_size)
      : _crossfade_data(
          iterator_type(index_iterator<T>()
            , math::raised_cosine<T>(static_cast<T>(2 * block_size))),
          // block_size + 1 because we also use it in reverse order
          iterator_type(index_iterator<T>(static_cast<T>(block_size + 1))))
      , _size(block_size)
    {}

    iterator fade_out_begin() const { return _crossfade_data.begin(); }
    reverse_iterator fade_in_begin() const { return _crossfade_data.rbegin(); }
    size_t size() const { return _size; }

  private:
    const std::vector<T> _crossfade_data;
    const size_t _size;
};

}  // namespace apf

#endif

// Settings for Vim (http://www.vim.org/), please do not remove:
// vim:softtabstop=2:shiftwidth=2:expandtab:textwidth=80:cindent
// vim:fdm=expr:foldexpr=getline(v\:lnum)=~'/\\*\\*'&&getline(v\:lnum)!~'\\*\\*/'?'a1'\:getline(v\:lnum)=~'\\*\\*/'&&getline(v\:lnum)!~'/\\*\\*'?'s1'\:'='
