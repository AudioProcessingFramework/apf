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
/// Some containers.

#ifndef APF_CONTAINER_H
#define APF_CONTAINER_H

#include <memory>  // for std::allocator
#include <list>  // for reordering logic of fixed_list
#include <stdexcept>  // for std::logic_error
#include <algorithm>  // for std::find

#include "apf/iterator.h"  // for cast_iterator, stride_iterator, ...

// apf::cast_iterator is (ab)used for the extra dereferenciation, the cast
// functionality is not used (and will be optimized away).
// Alternatively, boost::indirect_iterator could have been used.

namespace apf
{

/** Similar to std::vector, but without memory re-allocation and with the
 * ability to store non-copyable types.
 *
 * Properties:
 * - contiguous memory
 * - no re-allocations, memory locations never change
 * - non-copyable types can be used
 *
 * Main API differences to std::vector:
 * - different constructors
 * - no copy constructor
 * - no assignment operator, no assign(), no swap()
 * - no functions to change size except initialize() and emplace_back()
 * - no at() (just because of laziness, could be added easily)
 *
 * Iterators are never invalidated, however, the return values of end() and
 * rbegin() of course change after emplace_back().
 *
 * The fixed_vector is meant to be initialized with a fixed size which is never
 * changed during its lifetime. However, in some situations a fixed_vector has
 * to be created when its size is not yet known. In this case, it can be
 * created with the default constructor and initialized later with initialize().
 * Alternatively, memory can be allocated with allocate() and the initialization
 * can be done element by element with emplace_back().
 **/
template<typename T, typename Allocator = std::allocator<T> >
class fixed_vector
{
  private:
    // TODO: In C++11 there is std::enable_if and std::is_convertible!
    template <bool, typename X = void> struct _enable_if {};
    template <typename X> struct _enable_if<true, X> { typedef X type; };

    template <typename From, typename To>
    struct _is_convertible
    {
      private:
        struct _true { char dummy[2]; };
        struct _false {};
        static _true _check(const To&);
        static _false _check(...);

      public:
        static const bool value = (sizeof(_true) == sizeof(_check(From())));
    };

  public:
    // note: in C++11 there is an allocator_traits class.

    typedef T value_type;
    typedef Allocator allocator_type;
    typedef typename Allocator::size_type size_type;
    typedef typename Allocator::difference_type difference_type;
    typedef typename Allocator::reference reference;
    typedef typename Allocator::const_reference const_reference;
    typedef typename Allocator::pointer pointer;
    typedef typename Allocator::const_pointer const_pointer;

    typedef pointer iterator;
    typedef const_pointer const_iterator;

    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    /// Default constructor.
    /// Create a fixed_vector of size 0 and reserve 0 memory.
    /// The only useful thing to do after that is initialize() or allocate().
    explicit fixed_vector(const Allocator& a = Allocator())
      : _allocator(a)
      , _capacity(0)
    {
      this->allocate(0);
    }

    /// Allocate memory for @p n items and default-construct them.
    explicit fixed_vector(size_type n, const Allocator& a = Allocator())
      : _allocator(a)
      , _capacity(0)
    {
      this->initialize(n);
    }

    /// Allocate memory for @p n items and construct them using @p arg.
    template<typename Initializer>
    fixed_vector(size_type n, const Initializer& arg
        , const Allocator& a = Allocator())
      : _allocator(a)
      , _capacity(0)
    {
      this->initialize(n, arg);
    }

    /// Allocate memory for @p arg.first items and construct them using
    /// @p arg.second.
    /// This constructor is thought for initializing nested fixed_vector%s and
    /// fixed_list%s.
    // TODO: in C++11 this can maybe avoided using variadic templates?
    template<typename Initializer>
    explicit fixed_vector(const std::pair<size_type, Initializer>& arg
        , const Allocator& a = Allocator())
      : _allocator(a)
      , _capacity(0)
    {
      this->initialize(arg.first, arg.second);
    }

    /// Constructor from sequence. The sequence is not copied but rather used as
    /// constructor arguments for the new Ts.
    // TODO: In C++11 it's nicer to disable with a template argument:
    // template<typename InputIterator, typename = typename
    //   enable_if<!is_convertible<InputIterator, size_t>::value>::type>
    // In that case, the function signature doesn't change.
    template<typename In>
    fixed_vector(In first, In last, const Allocator& a = Allocator()
        , typename _enable_if<!_is_convertible<In, size_t>::value>::type* = nullptr)
      : _allocator(a)
      , _capacity(0)
    {
      this->initialize(first, last);
    }

    ~fixed_vector()
    {
      _destroy_and_deallocate();
    }

    /// Allocate memory and initialize with default constructor.
    /// @param n Number of elements to create
    /// @pre capacity() must be 0!
    /// @throw . whatever allocate() throws
    void initialize(size_type n)
    {
      this->allocate(n);

      // Allocator::construct() is not used because it would require T to have
      // a copy ctor.

      while (_size < _capacity) new (_data + _size++) T();
    }

    /// Allocate and initialize new data with given argument.
    /// @param n Number of elements to create
    /// @param arg Constructor argument
    /// @pre capacity() must be 0!
    /// @throw . whatever allocate() throws
    template<typename Initializer>
    void initialize(size_type n, const Initializer& arg)
    {
      this->allocate(n);

      while (_size < _capacity) new (_data + _size++) T(arg);
    }

    /// Allocate and initialize new data from sequence.
    /// @param first Iterator to argument for first element
    /// @param last Past-the-end iterator
    /// @pre capacity() must be 0!
    /// @throw . whatever allocate() throws
    // TODO: in C++11, use std::enable_if as template argument
    template<typename In>
    void initialize(In first, In last
        , typename _enable_if<!_is_convertible<In, size_t>::value>::type* = nullptr)
    {
      this->allocate(static_cast<size_type>(std::distance(first, last)));

      while (_size < _capacity) new (_data + _size++) T(*first++);
    }

    /// Allocate new memory.
    /// The memory will be uninitialized and the size() will be 0. Use
    /// emplace_back() to construct content.
    /// @pre capacity() must be 0!
    /// @throw std::logic_error if capacity is != 0.
    void allocate(size_type n)
    {
      if (_capacity != 0)
      {
        throw std::logic_error("fixed_vector::allocate: capacity must be 0!");
      }
      _size = 0;
      _capacity = n;
      _data = _allocator.allocate(_capacity);
    }

    /// Initialize a new item if reserved memory is available.
    /// @param arg Argument passed to the constructor of @p T
    /// @pre Enough memory must have been reserved with allocate().
    /// @throw std::out_of_range if not enough memory was reserved
    template<typename Arg>
    void emplace_back(const Arg& arg)
    {
      if (_size < _capacity)
      {
        new (_data + _size) T(arg);
        ++_size;
      }
      else
      {
        throw std::out_of_range(
            "fixed_vector::emplace_back(): capacity exceeded!");
      }
    }

    // TODO: emplace_back() with 2 arguments (or variadic template version)

#if 0
    // in C++11 this can be simplified with variadic templates:

    template<typename... Args>
    void emplace_back(Args&&... args)
    {
      if (_size < _capacity)
      {
        new (_data + _size) T(args...);
        ++_size;
      }
      else
      {
        throw std::out_of_range(
            "fixed_vector::emplace_back(): capacity exceeded!");
      }
    }
#endif

    template<typename Arg1, typename Arg2, typename Arg3>
    void emplace_back(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
    {
      if (_size < _capacity)
      {
        new (_data + _size) T(arg1, arg2, arg3);
        ++_size;
      }
      else
      {
        throw std::out_of_range(
            "fixed_vector::emplace_back(): capacity exceeded!");
      }
    }

    iterator begin() { return _data; };
    iterator end() { return _data + _size; };
    const_iterator begin() const { return _data; };
    const_iterator end() const { return _data + _size; };

    reverse_iterator rbegin() { return reverse_iterator(_data + _size); };
    reverse_iterator rend() { return reverse_iterator(_data); };
    const_reverse_iterator rbegin() const { return reverse_iterator(_data + _size); };
    const_reverse_iterator rend() const { return reverse_iterator(_data); };

    /// Size of initialized items. If more memory was reserved with allocate(),
    /// it can be initialized with emplace_back().
    /// @see capacity()
    size_type size() const { return _size; };

    /// Allocated space. If size() < capacity(), new items can be added with
    /// emplace_back().
    size_type capacity() const { return _capacity; };

    bool empty() const { return _size == 0; };

    reference operator[](size_type n) { return _data[n]; }
    const_reference operator[](size_type n) const { return _data[n]; }

    reference front() { return _data[0]; }
    reference back() { return _data[_size - 1]; }
    const_reference front() const { return _data[0]; }
    const_reference back() const { return _data[_size - 1]; }

    allocator_type get_allocator() const { return _allocator; }

  private:
    fixed_vector(const fixed_vector&);  // deactivated
    fixed_vector& operator=(const fixed_vector&);  // deactivated

    void _destroy_and_deallocate()
    {
      // Allocator::destroy() is not used for symmetry
      while (_size > 0)
      {
        --_size;
        (_data + _size)->~T();  // call destructor
      }
      _allocator.deallocate(_data, _capacity);
      _capacity = 0;
    }

    allocator_type _allocator;
    size_type _size, _capacity;
    pointer _data;
};

/** Vaguely similar to std::list, but without re-sizing.
 * Contrary to std::list, apf::fixed_list can be used with non-copyable
 * types.
 *
 * Properties:
 * - using apf::fixed_vector for storage
 * - using std::list for access
 * - items cannot be added/removed, but they can be re-ordered
 * - non-copyable types can be used
 *
 * Main API differences to std::list:
 * - no default constructor
 * - a different constructor with given size (no default content)
 *   - contained type needs to have default constructor!
 * - a different constructor from sequence
 *   - no copy but construction from given argument
 * - no copy constructor
 * - no assignment operator, no assign(), no swap()
 * - no functions to change size and related stuff
 * - no splice() but move(), which uses std::list::splice() internally
 * - no sort()/reverse(), but this could be implemented with splice() / move()
 **/
template<typename T, typename Allocator = std::allocator<T> >
class fixed_list
{
  private:
    typedef fixed_vector<T, Allocator> _data_type;
    // std::list is used to avoid re-implementing doubly-linked-list-logic.
    typedef std::list<T*> _access_type;

  public:
    typedef T value_type;
    typedef Allocator allocator_type;
    typedef typename _data_type::size_type size_type;
    typedef typename _data_type::difference_type difference_type;
    typedef typename _data_type::reference reference;
    typedef typename _data_type::const_reference const_reference;
    typedef typename _data_type::pointer pointer;
    typedef typename _data_type::const_pointer const_pointer;

    typedef cast_iterator<T, typename _access_type::iterator> iterator;
    typedef cast_iterator<T, typename _access_type::const_iterator>
      const_iterator;

    typedef cast_iterator<T, typename _access_type::reverse_iterator>
      reverse_iterator;
    typedef cast_iterator<T, typename _access_type::const_reverse_iterator>
      const_reverse_iterator;

    // TODO: Default ctor and fixed_list::initialize()?

    explicit fixed_list(size_type n, const Allocator& a = Allocator())
      : _data(n, a)
    {
      _update_access();
    }

    template<typename Size, typename Initializer>
    explicit fixed_list(const std::pair<Size, Initializer>& arg
        , const Allocator& a = Allocator())
      : _data(arg, a)
    {
      _update_access();
    }

    template<typename In>
    fixed_list(In first, In last, const Allocator& a = Allocator())
      : _data(first, last, a)
    {
      _update_access();
    }

    /// Move list element @p from one place @p to another.
    /// @p from is placed in front of @p to.
    /// No memory is allocated/deallocated, no content is copied.
    void move(iterator from, iterator to)
    {
      _access.splice(to.base(), _access, from.base());
    }

    /// Move range (from @p first to @p last) to @p target.
    /// The range is placed in front of @p target.
    /// No memory is allocated/deallocated, no content is copied.
    void move(iterator first, iterator last, iterator target)
    {
      _access.splice(target.base(), _access, first.base(), last.base());
    }

    iterator
    begin() { return make_cast_iterator<T>(_access.begin()); };
    iterator
    end()   { return make_cast_iterator<T>(_access.end()); };
    const_iterator
    begin() const { return make_cast_iterator<T>(_access.begin()); };
    const_iterator
    end() const { return make_cast_iterator<T>(_access.end()); };
    reverse_iterator
    rbegin() {return make_cast_iterator<T>(_access.rbegin());};
    reverse_iterator
    rend() {return make_cast_iterator<T>(_access.rend());};
    const_reverse_iterator
    rbegin() const { return make_cast_iterator<T>(_access.rbegin()); };
    const_reverse_iterator
    rend() const { return make_cast_iterator<T>(_access.rend()); };

    size_type size() const { return _data.size(); };
    bool empty() const { return _data.empty(); };

    reference front() { return *_access.front(); }
    reference back() { return *_access.back(); }
    const_reference front() const { return *_access.front(); }
    const_reference back() const { return *_access.back(); }

    allocator_type get_allocator() const { return _data.get_allocator(); }

  private:
    fixed_list(const fixed_list&);  // deactivated
    fixed_list& operator=(const fixed_list&);  // deactivated

    void _update_access()
    {
      for (T* ptr = _data.begin(); ptr != _data.end(); ++ptr)
      {
        _access.push_back(ptr);
      }
    }

    _data_type _data;
    _access_type _access;
};

/** Two-dimensional data storage for row- and column-wise access.
 * The two dimensions have following properties:
 *   -# Channel
 *     - stored in contiguous memory
 *     - fixed_matrix can be iterated from channels.begin() to channels.end()
 *       (using fixed_matrix::channels_iterator)
 *     - resulting channel can be iterated from .begin() to .end()
 *       (using fixed_matrix::channel_iterator)
 *   -# Slice
 *     - stored in memory locations with constant step size
 *     - fixed_matrix can be iterated from slices.begin() to slices.end()
 *       (using fixed_matrix::slices_iterator)
 *     - resulting slice can be iterated from .begin() to .end()
 *       (using fixed_matrix::slice_iterator)
 *
 * @tparam T Type of stored data
 **/
template<typename T, typename Allocator = std::allocator<T> >
class fixed_matrix : public fixed_vector<T, Allocator>
{
  private:
    typedef fixed_vector<T, Allocator> _base;

  public:
    typedef typename _base::pointer pointer;
    typedef typename _base::size_type size_type;

    /// Proxy class for returning one channel of the fixed_matrix
    typedef has_begin_and_end<pointer> Channel;
    /// Iterator within a Channel
    typedef typename Channel::iterator channel_iterator;

    /// Proxy class for returning one slice of the fixed_matrix
    typedef has_begin_and_end<stride_iterator<channel_iterator> > Slice;
    /// Iterator within a Slice
    typedef typename Slice::iterator slice_iterator;

    class channels_iterator;
    class slices_iterator;

    /// Default constructor.
    /// Only initialize() and allocate() make sense after this.
    explicit fixed_matrix(const Allocator& a = Allocator())
      : fixed_vector<T, Allocator>(0, a)
      , channels(channels_iterator(this->begin(), 0), 0)
      , slices(slices_iterator(this->begin(), 0, 0), 0)
      , _channel_ptrs(0)
    {}

    /** Constructor.
     * @param max_channels Number of Channels
     * @param max_slices Number of Slices
     * @param a Optional allocator
     **/
    fixed_matrix(size_type max_channels, size_type max_slices
        , const Allocator& a = Allocator())
      : fixed_vector<T, Allocator>(a)
    {
      this->initialize(max_channels, max_slices);
    }

    /// Allocate memory for @p max_channels x @p max_slices elements and
    /// default-construct them.
    /// @pre empty() == true
    /// @throw . whatever fixed_vector::initialize() throws
    void initialize(size_type max_channels, size_type max_slices)
    {
      assert(max_channels > 0);
      assert(max_slices   > 0);

      this->fixed_vector<T, Allocator>::initialize(max_channels * max_slices);
      this->channels = has_begin_and_end<channels_iterator>(
          channels_iterator(this->begin(), max_slices), max_channels);
      this->slices = has_begin_and_end<slices_iterator>(
          slices_iterator(this->begin(), max_channels, max_slices), max_slices);
      _channel_ptrs.allocate(max_channels);

      for (channels_iterator it = this->channels.begin()
          ; it != this->channels.end()
          ; ++it)
      {
        _channel_ptrs.emplace_back(it->begin());
      }
    }

    template<typename Ch>
    void set_channels(const Ch& ch);

    /// Get array of pointers to the channels. This can be useful to interact
    /// with functions which use plain pointers instead of iterators.
    pointer const* get_channel_ptrs() const { return _channel_ptrs.begin(); }

    /// Access to Channels; use channels.begin() and channels.end()
    has_begin_and_end<channels_iterator> channels;
    /// Access to Slices; use slices.begin() and slices.end()
    has_begin_and_end<slices_iterator>   slices;

  private:
    fixed_vector<pointer> _channel_ptrs;
};

/// Iterator over fixed_matrix::Channel%s.
template<typename T, typename Allocator>
class fixed_matrix<T, Allocator>::channels_iterator
{
  private:
    typedef channels_iterator self;
    typedef stride_iterator<channel_iterator> _base_type;

    /// Helper class for operator->()
    struct ChannelArrowProxy : Channel
    {
      ChannelArrowProxy(const Channel& ch) : Channel(ch) {}
      Channel* operator->() { return this; }
    };

  public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef Channel value_type;
    typedef Channel reference;
    typedef typename _base_type::difference_type difference_type;
    typedef ChannelArrowProxy pointer;

    /// Default constructor.
    /// @note This constructor creates a singular iterator. Another
    /// channels_iterator can be assigned to it, but nothing else works.
    channels_iterator()
      : _size(0)
    {}

    /// Constructor.
    channels_iterator(channel_iterator base_iterator, size_type step)
      : _base_iterator(base_iterator, step)
      , _size(step)
    {}

    /// Dereference operator.
    /// @return a proxy object of type fixed_matrix::Channel
    reference operator*() const
    {
      channel_iterator temp = _base_iterator.base();
      assert(apf::no_nullptr(temp));
      return Channel(temp, temp + _size);
    }

    /// Arrow operator.
    /// @return a proxy object of type fixed_matrix::ChannelArrowProxy
    pointer operator->() const
    {
      return this->operator*();
    }

    APF_ITERATOR_RANDOMACCESS_EQUAL(_base_iterator)
    APF_ITERATOR_RANDOMACCESS_PREINCREMENT(_base_iterator)
    APF_ITERATOR_RANDOMACCESS_PREDECREMENT(_base_iterator)
    APF_ITERATOR_RANDOMACCESS_ADDITION_ASSIGNMENT(_base_iterator)
    APF_ITERATOR_RANDOMACCESS_DIFFERENCE(_base_iterator)
    APF_ITERATOR_RANDOMACCESS_SUBSCRIPT
    APF_ITERATOR_RANDOMACCESS_LESS(_base_iterator)
    APF_ITERATOR_RANDOMACCESS_UNEQUAL
    APF_ITERATOR_RANDOMACCESS_OTHER_COMPARISONS
    APF_ITERATOR_RANDOMACCESS_POSTINCREMENT
    APF_ITERATOR_RANDOMACCESS_POSTDECREMENT
    APF_ITERATOR_RANDOMACCESS_THE_REST

    APF_ITERATOR_BASE(_base_type, _base_iterator)

  private:
    _base_type _base_iterator;
    size_type _size;
};

/// Iterator over fixed_matrix::Slice%s.
template<typename T, typename Allocator>
class fixed_matrix<T, Allocator>::slices_iterator
{
  private:
    typedef slices_iterator self;

    /// Helper class for operator->()
    struct SliceArrowProxy : Slice
    {
      SliceArrowProxy(const Slice& sl) : Slice(sl) {}
      Slice* operator->() { return this; }
    };

  public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef Slice value_type;
    typedef Slice reference;
    typedef typename std::iterator_traits<channel_iterator>::difference_type
      difference_type;
    typedef SliceArrowProxy pointer;

    /// Default constructor.
    /// @note This constructor creates a singular iterator. Another
    /// slices_iterator can be assigned to it, but nothing else works.
    slices_iterator()
      : _max_channels(0)
      , _max_slices(0)
    {}

    /// Constructor.
    slices_iterator(channel_iterator base_iterator
        , size_type max_channels, size_type max_slices)
      : _base_iterator(base_iterator)
      , _max_channels(max_channels)
      , _max_slices(max_slices)
    {}

    /// Dereference operator.
    /// @return a proxy object of type fixed_matrix::Slice
    reference operator*() const
    {
      assert(apf::no_nullptr(_base_iterator));
      slice_iterator temp(_base_iterator, _max_slices);
      return Slice(temp, temp + _max_channels);
    }

    /// Arrow operator.
    /// @return a proxy object of type fixed_matrix::SliceArrowProxy
    pointer operator->() const
    {
      return this->operator*();
    }

    APF_ITERATOR_RANDOMACCESS_EQUAL(_base_iterator)
    APF_ITERATOR_RANDOMACCESS_PREINCREMENT(_base_iterator)
    APF_ITERATOR_RANDOMACCESS_PREDECREMENT(_base_iterator)
    APF_ITERATOR_RANDOMACCESS_ADDITION_ASSIGNMENT(_base_iterator)
    APF_ITERATOR_RANDOMACCESS_DIFFERENCE(_base_iterator)
    APF_ITERATOR_RANDOMACCESS_SUBSCRIPT
    APF_ITERATOR_RANDOMACCESS_LESS(_base_iterator)
    APF_ITERATOR_RANDOMACCESS_UNEQUAL
    APF_ITERATOR_RANDOMACCESS_OTHER_COMPARISONS
    APF_ITERATOR_RANDOMACCESS_POSTINCREMENT
    APF_ITERATOR_RANDOMACCESS_POSTDECREMENT
    APF_ITERATOR_RANDOMACCESS_THE_REST

    APF_ITERATOR_BASE(channel_iterator, _base_iterator)

  private:
    channel_iterator _base_iterator;
    size_type _max_channels;
    size_type _max_slices;
};

/** Copy channels from another matrix.
 * @param ch channels (or slices) to copy from another fixed_matrix
 * @note A plain copy may be faster with @c std::copy() from
 *   fixed_matrix::begin() to fixed_matrix::end().
 * @note Anyway, a plain copy of a fixed_matrix is rarely needed, the main
 *   reason for this function is that if you use slices instead of channels,
 *   you'll get a transposed matrix.
 * @pre The dimensions must be correct beforehand!
 * @warning If the dimensions are not correct, bad things will happen!
 **/
template<typename T, typename Allocator>
template<typename Ch>
void
fixed_matrix<T, Allocator>::set_channels(const Ch& ch)
{
  assert(std::distance(ch.begin(), ch.end())
      == std::distance(this->channels.begin(), this->channels.end()));
  assert((ch.begin() == ch.end()) ? true :
      std::distance(ch.begin()->begin(), ch.begin()->end()) ==
      std::distance(this->channels.begin()->begin()
                  , this->channels.begin()->end()));

  channels_iterator target = this->channels.begin();

  for (typename Ch::iterator it = ch.begin()
      ; it != ch.end()
      ; ++it)
  {
    std::copy(it->begin(), it->end(), target->begin());
    ++target;
  }
}

/// Append pointers to the elements of the first list to the second list.
/// @note @c L2::value_type must be a pointer to @c L1::value_type!
template<typename L1, typename L2>
void append_pointers(L1& source, L2& target)
{
  for (typename L1::iterator it = source.begin()
      ; it != source.end()
      ; ++it)
  {
    target.push_back(&*it);
  }
}

/// Const-version of append_pointers()
/// @note @c L2::value_type must be a pointer to @b const @c L1::value_type!
template<typename L1, typename L2>
void append_pointers(const L1& source, L2& target)
{
  for (typename L1::const_iterator it = source.begin()
      ; it != source.end()
      ; ++it)
  {
    target.push_back(&*it);
  }
}

/// Splice list elements from @p source to member lists of @p target.
/// @param source Elements of this list are distributed to the corresponding
///   @p member lists of @p target. This must have the same type as @p member.
/// @param target Each element of this list receives one element of @p source.
/// @param member The distributed elements are appended at @c member.end().
/// @note Lists must have the same size. If not, an exception is thrown.
/// @note There is no const version, both lists are modified.
/// @post @p source will be empty.
/// @post The @p member of each @p target element will have one more element.
template<typename L1, typename L2, typename DataMember>
void distribute_list(L1& source, L2& target, DataMember member)
{
  if (source.size() != target.size())
  {
    throw std::logic_error("distribute_list: Different sizes!");
  }

  typename L1::iterator in = source.begin();

  for (typename L2::iterator out = target.begin()
      ; out != target.end()
      ; ++out)
  {
    ((*out).*member).splice(((*out).*member).end(), source, in++);
  }
}

/// The opposite of distribute_list() -- sorry for the strange name!
/// @param source Container of items which will be removed from @p member of the
///   corresponding @p target elements.
/// @param target Container of elements which have a @p member.
/// @param member Member container from which elements will be removed. Must
///   have a splice() member function (like @c std::list).
/// @param garbage Removed elements are appended to this list. Must have the
///   same type as @p member.
/// @throw std::logic_error If any element isn't found in the corresponding
///   @p member.
/// @attention If a list element is not found, an exception is thrown and the
///   original state is @b not restored!
// TODO: better name?
template<typename L1, typename L2, typename DataMember, typename L3>
void
undistribute_list(const L1& source, L2& target, DataMember member, L3& garbage)
{
  if (source.size() != target.size())
  {
    throw std::logic_error("undistribute_list(): Different sizes!");
  }

  typename L1::const_iterator in = source.begin();

  for (typename L2::iterator out = target.begin()
      ; out != target.end()
      ; ++out)
  {
    typename L3::iterator delinquent
      = std::find(((*out).*member).begin(), ((*out).*member).end(), *in++);
    if (delinquent == ((*out).*member).end())
    {
      throw std::logic_error("undistribute_list(): Element not found!");
    }
    garbage.splice(garbage.end(), (*out).*member, delinquent);
  }
}

}  // namespace apf

#endif

// Settings for Vim (http://www.vim.org/), please do not remove:
// vim:softtabstop=2:shiftwidth=2:expandtab:textwidth=80:cindent
// vim:fdm=expr:foldexpr=getline(v\:lnum)=~'/\\*\\*'&&getline(v\:lnum)!~'\\*\\*/'?'a1'\:getline(v\:lnum)=~'\\*\\*/'&&getline(v\:lnum)!~'/\\*\\*'?'s1'\:'='
