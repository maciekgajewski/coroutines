// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_GLOBALS_HPP
#define COROUTINES_GLOBALS_HPP

#include "scheduler.hpp"
#include "context.hpp"

// global functions used in channle-based concurent programming

namespace coroutines
{

extern scheduler* __scheduler;


inline void set_scheduler(scheduler* sched) { __scheduler = sched; }

template<typename Callable, typename... Args>
void go(std::string name, Callable&& fn, Args&&... args)
{
    assert(__scheduler);
    __scheduler->go(name, std::forward<Callable>(fn), std::forward<Args>(args)...);
}

template<typename Callable, typename... Args>
void go(const char* name, Callable&& fn, Args&&... args)
{
    assert(__scheduler);
    __scheduler->go(std::string(name), std::forward<Callable>(fn), std::forward<Args>(args)...);
}

template<typename Callable, typename... Args>
void go(Callable&& fn, Args&&... args)
{
    assert(__scheduler);
    __scheduler->go(std::forward<Callable>(fn), std::forward<Args>(args)...);
}

// create channel
template<typename T>
channel_pair<T> make_channel(std::size_t capacity)
{
    assert(__scheduler);
    return __scheduler->make_channel<T>(capacity);
}

// begin blocking operation
// starting coroutines is not allowed in blocking mode
inline void block()
{
    context* ctx = context::current_context();
    assert(ctx);
    ctx->block();
}

// ends blocking mode. may preempt current coroutine
inline void unblock()
{
    context* ctx = context::current_context();
    assert(ctx);
    ctx->unblock();
}


}

#endif
