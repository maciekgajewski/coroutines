// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_COROUTINE_SCHEDULER_HPP
#define COROUTINES_COROUTINE_SCHEDULER_HPP

#include "coroutines/channel.hpp"
#include "coroutines/coroutine.hpp"
#include "coroutines/context.hpp"
#include "coroutines/locking_channel.hpp"
#include "coroutines/condition_variable.hpp"
#include "coroutines/thread_safe_queue.hpp"
#include "coroutines/thread_pool.hpp"

#include <thread>
#include <mutex>
#include <list>

namespace coroutines {

class scheduler
{
public:
    // creates and sets no of max coroutines runnig in parallel
    scheduler(unsigned max_running_coroutines);

    scheduler(const scheduler&) = delete;

    ~scheduler();

    // launches corountine
    template<typename Callable, typename... Args>
    void go(Callable&& fn, Args&&... args);

    // debug version, with coroutine's name
    template<typename Callable, typename... Args>
    void go(std::string name, Callable&& fn, Args&&... args);

    // debug version, with coroutine's name
    template<typename Callable, typename... Args>
    void go(const char* name, Callable&& fn, Args&&... args);

    // create channel
    template<typename T>
    channel_pair<T> make_channel(std::size_t capacity, const std::string& name)
    {
        return locking_channel<T>::make(*this, capacity, name);
    }

    // wait for all coroutines to complete
    void wait();

    ///////
    // context's interface

    void get_all_from_global_queue(std::list<coroutine_ptr>& out)
    {
        _global_queue.get_all(out);
    }

    // steals half the queue of one of the active contexts
    void steal(std::list<coroutine_ptr>& out);

    // used by context to report it completed the job. Will destroy the context
    void context_finished(context* ctx);

    // used by context to singalize progression into blocked state
    void context_blocked(context* ctx, std::list<coroutine_ptr>& coros, const std::string& checkpoint_name);

    // used by context to singalize progression out of blocked state.
    // will activate context if resources available, or destroy it
    // returns 'true' is context is allowed to continue, false if it's going to be destroyed
    bool context_unblocked(context* ctx, const std::string& checkpoint_name);

    void schedule(coroutine_ptr&& coro);
    void schedule(std::list<coroutine_ptr>& coros);


private:


    std::vector<context_ptr> _blocked_contexts;
    std::vector<context_ptr> _active_contexts;
    std::mutex _contexts_mutex;
    const unsigned _max_running_coroutines;

    thread_safe_queue<coroutine_ptr> _global_queue; // coroutines not assigned to any context
    thread_pool _thread_pool;

};


template<typename Callable, typename... Args>
void scheduler::go(Callable&& fn, Args&&... args)
{
    go(std::string(), std::forward<Callable>(fn), std::forward<Args>(args)...);
}

template<typename Callable, typename... Args>
void scheduler::go(std::string name, Callable&& fn, Args&&... args)
{
    schedule(make_coroutine(std::move(name), std::bind(std::forward<Callable>(fn), std::forward<Args>(args)...)));
}

template<typename Callable, typename... Args>
void scheduler::go(const char* name, Callable&& fn, Args&&... args)
{
    schedule(make_coroutine(std::string(name), std::bind(std::forward<Callable>(fn), std::forward<Args>(args)...)));
}

} // namespace coroutines

#endif // COROUTINES_COROUTINE_SCHEDULER_HPP
