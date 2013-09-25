// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_GLOBALS_HPP
#define COROUTINES_GLOBALS_HPP

#include "threaded_scheduler.hpp"

// global functions used in channle-based concurent programming

namespace coroutines
{

extern threaded_scheduler* __scheduler;


inline void set_scheduler(threaded_scheduler* sched) { __scheduler = sched; }

template<typename Callable, typename... Args>
void go(Callable&& fn, Args&&... args)
{
    assert(__scheduler);
    __scheduler->go(std::forward<Callable>(fn), std::forward<Args>(args)...);
}

// create channek
template<typename T>
channel_pair<T> make_channel(std::size_t capacity)
{
    assert(__scheduler);
    return __scheduler->make_channel<T>(capacity);
}

}

#endif
