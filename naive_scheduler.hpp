// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_NAIVE_SCHEDULER_HPP
#define COROUTINES_NAIVE_SCHEDULER_HPP

#include <thread>
#include <functional>
#include <utility>

#include "naive_channel.hpp"

namespace corountines {

// Naive scheduler simply starts each 'corountine as a separate thread
class naive_scheduler
{
public:

    naive_scheduler();
    naive_scheduler(const naive_scheduler&) = delete;
    ~naive_scheduler();

    template<typename T>
    naive_channel<T> make_channel(std::size_t capacity);

    // launches corountine
    template<typename Callable, typename... Args>
    void go(Callable&& fn, Args&&... args);
};
    

template<typename T>
naive_channel<T> naive_scheduler::make_channel(std::size_t capacity)
{
    return naive_channel<T>(capacity);
}

template<typename Callable, typename... Args>
void naive_scheduler::go(Callable&& fn, Args&&... args)
{
    std::thread t(std::bind(std::forward<Callable>(fn), std::forward<Args>(args)...));
    t.detach();
}

}

#endif
