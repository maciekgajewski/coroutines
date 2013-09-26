#ifndef COROUTINES_COROUTINE_SCHEDULER_HPP
#define COROUTINES_COROUTINE_SCHEDULER_HPP

#include "channel.hpp"
#include "coroutine.hpp"
#include "context.hpp"
#include "coroutine_channel.hpp"
#include "condition_variable.hpp"
#include "thread_safe_queue.hpp"

#include <thread>
#include <mutex>
#include <list>

namespace coroutines {

class coroutine_scheduler
{
public:
    // creates and sets no of max coroutines runnig in parallel
    coroutine_scheduler(unsigned max_running_coroutines = 4);

    coroutine_scheduler(const coroutine_scheduler&) = delete;

    ~coroutine_scheduler();

    // launches corountine
    template<typename Callable, typename... Args>
    void go(Callable&& fn, Args&&... args);

    // debug version, with coroutine's name
    template<typename Callable, typename... Args>
    void go(std::string name, Callable&& fn, Args&&... args);

    // create channel
    template<typename T>
    channel_pair<T> make_channel(std::size_t capacity)
    {
        return coroutine_channel<T>::make(capacity);
    }

    // wait for all coroutines to complete
    void wait();

    void get_all_from_global_queue(std::list<coroutine>& out)
    {
        _global_queue.get_all(out);
    }

private:

    void schedule(coroutine&& coro);

    std::list<std::thread> _threads;
    std::mutex _threads_mutex;

    std::list<context> _idle_contexts;
    std::list<context> _active_contexts;
    std::mutex _contexts_mutex;

    thread_safe_queue<coroutine> _global_queue; // coroutines not assigned to any context

};


template<typename Callable, typename... Args>
void coroutine_scheduler::go(Callable&& fn, Args&&... args)
{
    go(std::string(), std::forward<Callable>(fn), std::forward<Args>(args)...);
}

template<typename Callable, typename... Args>
void coroutine_scheduler::go(std::string name, Callable&& fn, Args&&... args)
{
    schedule(make_coroutine(std::move(name), std::bind(std::forward<Callable>(fn), std::forward<Args>(args)...)));
}

} // namespace coroutines

#endif // COROUTINES_COROUTINE_SCHEDULER_HPP
