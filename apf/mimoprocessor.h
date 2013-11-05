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
/// Multi-threaded MIMO (multiple input, multiple output) processor.

#ifndef APF_MIMOPROCESSOR_H
#define APF_MIMOPROCESSOR_H

#include <cassert>  // for assert()
#include <stdexcept>  // for std::logic_error

#include "apf/rtlist.h"
#include "apf/parameter_map.h"
#include "apf/misc.h"  // for NonCopyable
#include "apf/iterator.h" // for *_iterator, make_*_iterator(), cast_proxy_const
#include "apf/container.h"  // for fixed_vector

#define APF_MIMOPROCESSOR_TEMPLATES template<typename Derived, typename interface_policy, typename thread_policy, typename query_policy>
#define APF_MIMOPROCESSOR_BASE MimoProcessor<Derived, interface_policy, thread_policy, query_policy>

#ifndef APF_MIMOPROCESSOR_DEFAULT_THREADS
#define APF_MIMOPROCESSOR_DEFAULT_THREADS 1
#endif

/** Macro to create a @c Process struct and a corresponding member function.
 * @param name Name of the containing class
 * @param parent Parent class (must have an inner class @c Process).
 *   The class apf::MimoProcessor::ProcessItem can be used.
 *
 * Usage examples:
 *                                                                         @code
 * // In 'public' section of Output:
 * APF_PROCESS(Output, MimoProcessorBase::Output)
 * {
 *   // do something here, you have access to members of Output
 * }
 *
 * // In 'public' section of MyItem:
 * APF_PROCESS(MyItem, ProcessItem<MyItem>)
 * {
 *   // do something here, you have access to members of MyItem
 *   // MyItem has to be publicly derived from ProcessItem<MyItem>
 * }
 *                                                                      @endcode
 **/
#define APF_PROCESS(name, parent) \
struct Process : parent::Process { \
  explicit Process(name& name ## _arg) : parent::Process(name ## _arg) { \
    name ## _arg._ ## name ## _process(); } }; \
void _ ## name ## _process()

namespace apf
{

// the default implementation does nothing
template<typename interface_policy, typename native_handle_type>
struct thread_traits
{
  static void set_priority(const interface_policy&, native_handle_type) {}
};

class enable_queries
{
  protected:
    enable_queries(int fifo_size) : _query_fifo(fifo_size) {}
    CommandQueue _query_fifo;
};

class disable_queries
{
  protected:
    disable_queries(int) {}
    struct { void process_commands() {} } _query_fifo;
};

/** Multi-threaded multiple-input-multiple-output (MIMO) processor.
 * Derive your own class from MimoProcessor and also use it as first template
 * argument. This is called the "Curiously Recurring Template Pattern" (CRTP).
 * The rest of the template arguments are \ref apf_policies ("Policy-based
 * Design").
 *
 * @tparam Derived Your derived class -> CRTP!
 * @tparam interface_policy Policy class. You can use existing policies (e.g.
 *   jack_policy, pointer_policy<T*>) or write your own policy class.
 * @tparam thread_policy Policy for threads, locks and semaphores.
 *
 * Example: @ref MimoProcessor
 **/
template<typename Derived
  , typename interface_policy, typename thread_policy
  , typename query_policy = disable_queries>
class MimoProcessor : public interface_policy
                    , public thread_policy
                    , public query_policy
                    , NonCopyable
{
  public:
    typedef typename interface_policy::sample_type sample_type;
    using query_policy::_query_fifo;

    class Input;
    class Output;
    class DefaultInput;
    class DefaultOutput;

    /// Abstract base class for list items.
    struct Item : NonCopyable
    {
      virtual ~Item() {}

      /// to be overwritten in the derived class
      virtual void process() = 0;
    };

    /** Base class for items which have a @c Process class.
     * Usage:
     *                                                                     @code
     * class MyItem : public ProcessItem<MyItem>
     * {
     *   public:
     *     APF_PROCESS(MyItem, ProcessItem<MyItem>)
     *     {
     *       // do something here, you have access to members of MyItem
     *     }
     * };
     *                                                                  @endcode
     * Multiple layers of inheritance are possible, but ProcessItem must
     * be instantiated with the most derived class!
     **/
    template<typename X>
    struct ProcessItem : Item
    {
      struct Process { explicit Process(Item&) {} };

      virtual void process()
      {
        assert(dynamic_cast<X*>(this));
        typename X::Process(*static_cast<X*>(this));
      }
    };

    typedef RtList<Item*> rtlist_t;

    /// Proxy class for accessing an RtList.
    /// @note This is for read-only access. Write access is only allowed in the
    ///   process() member function from within the object itself.
    template<typename T>
    struct rtlist_proxy : cast_proxy_const<T, rtlist_t>
    {
      rtlist_proxy(const rtlist_t& l) : cast_proxy_const<T, rtlist_t>(l) {}
    };

    /// Lock is released when it goes out of scope.
    class ScopedLock : NonCopyable
    {
      public:
        explicit ScopedLock(typename thread_policy::Lock& obj)
          : _obj(obj)
        {
          _obj.lock();
        }

        ~ScopedLock() { _obj.unlock(); }

      private:
        typename thread_policy::Lock& _obj;
    };

    class QueryThread
    {
      public:
        QueryThread(CommandQueue& fifo) : _fifo(fifo) {};
        void operator()() { _fifo.cleanup_commands(); }

      private:
        CommandQueue& _fifo;
    };

    template<typename F>
    class QueryCommand : public CommandQueue::Command
    {
      public:
        QueryCommand(F& query_function, Derived& parent)
          : _query_function(query_function)
          , _parent(parent)
        {}

        virtual void execute()
        {
          _query_function.query();
        }

        virtual void cleanup()
        {
          _query_function.update();
          _parent.new_query(_query_function);  // "recursive" call!
        }

      private:
        F& _query_function;
        Derived& _parent;
    };

    template<typename F>
    void new_query(F& query_function)
    {
      _query_fifo.push(new QueryCommand<F>(query_function
            , *static_cast<Derived*>(this)));
    }

    bool activate()
    {
      _fifo.reactivate();  // no return value
      return interface_policy::activate();
    }

    bool deactivate()
    {
      if (!interface_policy::deactivate()) return false;

      // All audio threads should be stopped now.

      // Inputs/Outputs push commands in their destructors -> we need a loop.
      do
      {
        // Exceptionally, this is called from the non-realtime thread:
        _fifo.process_commands();
        _fifo.cleanup_commands();
      }
      while (_fifo.commands_available());
      // The queue should be empty now.
      if (!_fifo.deactivate()) throw std::logic_error("Bug: FIFO not empty!");
      return true;

      // The lists can now be manipulated safely from the non-realtime thread.
    }

    void wait_for_rt_thread() { _fifo.wait(); }

    template<typename X>
    X* add()
    {
      return this->add(typename X::Params());
    }

    // TODO: find a way to get the outer type automatically
    template<typename P>
    typename P::outer* add(const P& p)
    {
      typedef typename P::outer X;
      P temp = p;
      assert(dynamic_cast<Derived*>(this));
      temp.parent = static_cast<Derived*>(this);
      return static_cast<X*>(_add_helper(new X(temp)));
    }

    void rem(Input* in) { _input_list.rem(in); }
    void rem(Output* out) { _output_list.rem(out); }

    const rtlist_t& get_input_list() const { return _input_list; }
    const rtlist_t& get_output_list() const { return _output_list; }

    const parameter_map params;

    template<typename F>
    static typename thread_policy::template ScopedThread<F>*
    new_scoped_thread(F f, typename thread_policy::useconds_type usleeptime)
    {
      return new typename thread_policy::template ScopedThread<F>(f,usleeptime);
    }

  protected:
    typedef APF_MIMOPROCESSOR_BASE MimoProcessorBase;

    typedef typename rtlist_t::size_type      size_type;
    typedef typename rtlist_t::iterator       rtlist_iterator;
    typedef typename rtlist_t::const_iterator rtlist_const_iterator;

    struct Process { Process(MimoProcessorBase&) {} };

    explicit MimoProcessor(const parameter_map& params = parameter_map());

    /// Protected non-virtual destructor
    ~MimoProcessor()
    {
      this->deactivate();
      _input_list.clear();
      _output_list.clear();
    }

    void _process_list(rtlist_t& l);
    void _process_list(rtlist_t& l1, rtlist_t& l2);

    CommandQueue _fifo;

  private:
    class WorkerThreadFunction;

    class WorkerThread : NonCopyable
    {
      private:
        typedef typename thread_policy::template DetachedThread<
          WorkerThreadFunction> Thread;

      public:
        WorkerThread(std::pair<int, MimoProcessor*> in)
          : cont_semaphore(0)
          , wait_semaphore(0)
          , _thread(WorkerThreadFunction(in.first, *in.second, *this))
        {
          // Set thread priority from interface_policy, if available
          thread_traits<interface_policy
            , typename Thread::native_handle_type>::set_priority(*in.second
                , _thread.native_handle());
        }

        typename thread_policy::Semaphore cont_semaphore;
        typename thread_policy::Semaphore wait_semaphore;

      private:
        Thread _thread;  // Thread must be initialized after semaphores
    };

    class WorkerThreadFunction
    {
      public:
        WorkerThreadFunction(int thread_number, MimoProcessor& parent
            , WorkerThread& thread)
          : _thread_number(thread_number)
          , _parent(parent)
          , _thread(thread)
        {}

        void operator()()
        {
          // wait for main thread
          _thread.cont_semaphore.wait();

          _parent._process_selected_items_in_current_list(_thread_number);

          // report to main thread
          _thread.wait_semaphore.post();
        }

      private:
        int _thread_number;
        MimoProcessor& _parent;
        WorkerThread& _thread;
    };

    class thread_init_helper;
    class Xput;

    // This is called from the interface_policy
    virtual void process()
    {
      _fifo.process_commands();
      _process_list(_input_list);
      assert(dynamic_cast<Derived*>(this));
      typename Derived::Process(*static_cast<Derived*>(this));
      _process_list(_output_list);
      _query_fifo.process_commands();
    }

    void _process_current_list_in_main_thread();
    void _process_selected_items_in_current_list(int thread_number);

    Input* _add_helper(Input* in) { return _input_list.add(in); }
    Output* _add_helper(Output* out) { return _output_list.add(out); }

    template<typename T> struct _empty_type {};

    // TODO: make "volatile"?
    rtlist_t* _current_list;

    /// Number of threads (main thread plus worker threads)
    const int _num_threads;

    fixed_vector<WorkerThread> _thread_data;

    rtlist_t _input_list, _output_list;
};

APF_MIMOPROCESSOR_TEMPLATES
class APF_MIMOPROCESSOR_BASE::thread_init_helper
{
  public:
    typedef std::pair<int, MimoProcessor*> result_type;

    explicit thread_init_helper(MimoProcessor* parent)
      : _parent(parent)
    {
      assert(_parent);
    }

    result_type operator()(int in)
    {
      return std::make_pair(in, _parent);
    }

  private:
    MimoProcessor* _parent;
};

/// @throw std::logic_error if CommandQueue cannot be deactivated.
APF_MIMOPROCESSOR_TEMPLATES
APF_MIMOPROCESSOR_BASE::MimoProcessor(const parameter_map& params_)
  : interface_policy(params_)
  , query_policy(params_.get("fifo_size", 1024))
  , params(params_)
  , _fifo(params.get("fifo_size", 1024))
  , _current_list(nullptr)
  , _num_threads(params.get("threads", APF_MIMOPROCESSOR_DEFAULT_THREADS))
  // Create worker threads.  NOTE: Number 0 is reserved for the main thread.
  , _thread_data(
      make_transform_iterator(make_index_iterator(1)
        , thread_init_helper(this)),
      make_transform_iterator(make_index_iterator(_num_threads)
        , thread_init_helper(this)))
  , _input_list(_fifo)
  , _output_list(_fifo)
{
  assert(_num_threads > 0);

  // deactivate FIFO for non-realtime initializations
  if (!_fifo.deactivate()) throw std::logic_error("Bug: FIFO not empty!");
}

APF_MIMOPROCESSOR_TEMPLATES
void
APF_MIMOPROCESSOR_BASE::_process_list(rtlist_t& l)
{
  _current_list = &l;
  _process_current_list_in_main_thread();
}

APF_MIMOPROCESSOR_TEMPLATES
void
APF_MIMOPROCESSOR_BASE::_process_list(rtlist_t& l1, rtlist_t& l2)
{
  // TODO: extend for more than two lists?

  // WARNING: this is NOT conforming to the current C++ standard!
  // According to C++03 iterators to the spliced elements are invalidated!
  // In C++1x this will be fixed.
  // see http://stackoverflow.com/q/143156

  // see also http://stackoverflow.com/q/7681376

  rtlist_iterator temp = l2.begin();
  l2.splice(temp, l1);  // join lists: "L2 = L1 + L2"
  _process_list(l2);
  l1.splice(l1.end(), l2, l2.begin(), temp);  // restore original lists

  // not exception-safe (original lists are not restored), but who cares?
}

APF_MIMOPROCESSOR_TEMPLATES
void
APF_MIMOPROCESSOR_BASE::_process_selected_items_in_current_list(int thread_number)
{
  assert(_current_list);

  int n = 0;
  for (rtlist_iterator i = _current_list->begin()
      ; i != _current_list->end()
      ; ++i, ++n)
  {
    if (thread_number == n % _num_threads)
    {
      (*i)->process();
    }
  }
}

APF_MIMOPROCESSOR_TEMPLATES
void
APF_MIMOPROCESSOR_BASE::_process_current_list_in_main_thread()
{
  assert(_current_list);
  if (_current_list->empty()) return;

  typedef typename fixed_vector<WorkerThread>::iterator thread_iterator;

  // wake all threads
  for (thread_iterator it = _thread_data.begin()
      ; it != _thread_data.end()
      ; ++it)
  {
    it->cont_semaphore.post();
  }

  _process_selected_items_in_current_list(0);

  // wait for worker threads
  for (thread_iterator it = _thread_data.begin()
      ; it != _thread_data.end()
      ; ++it)
  {
    it->wait_semaphore.wait();
  }
}

APF_MIMOPROCESSOR_TEMPLATES
class APF_MIMOPROCESSOR_BASE::Xput : public Item
{
  public:
    // Parameters for an Input or Output.
    // You can add your own parameters by deriving from it.
    struct Params : parameter_map
    {
      Params() : parent(nullptr) {}
      Derived* parent;

      Params& operator=(const parameter_map& p)
      {
        this->parameter_map::operator=(p);
        return *this;
      }
    };

    Derived& parent;

  protected:
    /// Protected Constructor.
    /// @throw std::logic_error if parent == NULL
    explicit Xput(const Params& p)
      : parent(*(p.parent
            ? p.parent
            : throw std::logic_error("Bug: In/Output: parent == 0!")))
    {}
};

/// %Input class.
APF_MIMOPROCESSOR_TEMPLATES
class APF_MIMOPROCESSOR_BASE::Input : public Xput
                                    , public interface_policy::Input
{
  public:
    struct Params : Xput::Params
    {
      using Xput::Params::operator=;
      typedef typename Derived::Input outer;  // see add()
    };

    struct Process { Process(Input&) {} };

    explicit Input(const Params& p)
      : Xput(p)
      , interface_policy::Input(*p.parent, p)
    {}

  private:
    virtual void process()
    {
      this->fetch_buffer();
      assert(dynamic_cast<typename Derived::Input*>(this));
      typename Derived::Input::Process(
          *static_cast<typename Derived::Input*>(this));
    }
};

/// %Input class with begin() and end().
APF_MIMOPROCESSOR_TEMPLATES
class APF_MIMOPROCESSOR_BASE::DefaultInput : public Input
{
  public:
    typedef typename Input::Params Params;
    typedef typename Input::iterator iterator;

    DefaultInput(const Params& p) : Input(p) {}

    iterator begin() const { return this->buffer.begin(); }
    iterator   end() const { return this->buffer.end(); }
};

/// %Output class.
APF_MIMOPROCESSOR_TEMPLATES
class APF_MIMOPROCESSOR_BASE::Output : public Xput
                                     , public interface_policy::Output
{
  public:
    struct Params : Xput::Params
    {
      using Xput::Params::operator=;
      typedef typename Derived::Output outer;  // see add()
    };

    struct Process { Process(Output&) {} };

    explicit Output(const Params& p)
      : Xput(p)
      , interface_policy::Output(*p.parent, p)
    {}

  private:
    virtual void process()
    {
      this->fetch_buffer();
      assert(dynamic_cast<typename Derived::Output*>(this));
      typename Derived::Output::Process(
          *static_cast<typename Derived::Output*>(this));
    }
};

/// %Output class with begin() and end().
APF_MIMOPROCESSOR_TEMPLATES
class APF_MIMOPROCESSOR_BASE::DefaultOutput : public Output
{
  public:
    typedef typename Output::Params Params;
    typedef typename Output::iterator iterator;

    DefaultOutput(const Params& p) : Output(p) {}

    iterator begin() const { return this->buffer.begin(); }
    iterator   end() const { return this->buffer.end(); }
};

}  // namespace apf

#undef APF_MIMOPROCESSOR_TEMPLATES
#undef APF_MIMOPROCESSOR_BASE

#endif

// Settings for Vim (http://www.vim.org/), please do not remove:
// vim:softtabstop=2:shiftwidth=2:expandtab:textwidth=80:cindent
// vim:fdm=expr:foldexpr=getline(v\:lnum)=~'/\\*\\*'&&getline(v\:lnum)!~'\\*\\*/'?'a1'\:getline(v\:lnum)=~'\\*\\*/'&&getline(v\:lnum)!~'/\\*\\*'?'s1'\:'='
