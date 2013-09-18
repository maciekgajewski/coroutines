// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_NAIVE_SCHEDULER_HPP
#define COROUTINES_NAIVE_SCHEDULER_HPP

#include <thread>
#include <functional>
#include <utility>
#include <list>

#include "threaded_channel.hpp"

namespace coroutines {
// Naive scheduler simply starts each corountine as separate thread
class threaded_scheduler
{
public:

    threaded_scheduler();
    threaded_scheduler(const threaded_scheduler&) = delete;
    ~threaded_scheduler();

    // launches corountine
    template<typename Callable, typename... Args>
    void go(Callable&& fn, Args&&... args);

    // create channek
    template<typename T>
    channel_pair<T> make_channel(std::size_t capacity) { return threaded_channel<T>::make(capacity); }

private:

    std::list<std::thread> _threads;
};
    

template<typename Callable, typename... Args>
void threaded_scheduler::go(Callable&& fn, Args&&... args)
{
    std::thread t(std::bind(std::forward<Callable>(fn), std::forward<Args>(args)...));

    _threads.push_back(std::move(t));
}
}

#endif
