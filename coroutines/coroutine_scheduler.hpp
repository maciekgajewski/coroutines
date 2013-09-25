#ifndef COROUTINES_COROUTINE_SCHEDULER_HPP
#define COROUTINES_COROUTINE_SCHEDULER_HPP

#include "channel.hpp"
#include "coroutine.hpp"
#include "context.hpp"
#include "threaded_channel.hpp"

#include <thread>
#include <mutex>
#include <list>

namespace coroutines {

class coroutine_scheduler
{
public:
    // creates and sets no of max coroutines runnig in parallel
    coroutine_scheduler(unsigned max_running_coroutines);

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
    channel_pair<T> make_channel(std::size_t capacity) { return threaded_channel<T>::make(capacity); }

    // wait for all coroutines to complete
    void wait();

private:

    std::list<std::thread> _threads;
    std::mutex _threads_mutex;

    std::list<context> _idle_contexts;
    std::list<context> _active_cotexts;
    std::mutex _contexts_mutex;

    std::list<coroutine> _global_queue; // coroutines not assigned to any context
    std::mutex _global_queue_mutex;

};


template<typename Callable, typename... Args>
void coroutine_scheduler::go(Callable&& fn, Args&&... args)
{
    go(std::string(), std::forward<Callable>(fn), std::forward<Args>(args)...);
}

template<typename Callable, typename... Args>
void coroutine_scheduler::go(std::string name, Callable&& fn, Args&&... args)
{
    std::lock_guard<std::mutex> lock(_threads_mutex);

    coroutine coro = make_coroutine(std::move(name), std::bind(std::forward<Callable>(fn), std::forward<Args>(args)...));
    // TODO launch it
}

} // namespace coroutines

#endif // COROUTINES_COROUTINE_SCHEDULER_HPP
