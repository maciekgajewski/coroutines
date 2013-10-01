// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_GLOBALS_HPP
#define COROUTINES_GLOBALS_HPP

#include "scheduler.hpp"
#include "context.hpp"

// global functions used in channle-based concurent programming

namespace coroutines
{

void set_scheduler(scheduler* sched);
scheduler* get_scheduler();
scheduler& get_scheduler_check(); // asserts scheduler not null

template<typename Callable, typename... Args>
void go(std::string name, Callable&& fn, Args&&... args)
{
    get_scheduler_check().go(name, std::forward<Callable>(fn), std::forward<Args>(args)...);
}

template<typename Callable, typename... Args>
void go(const char* name, Callable&& fn, Args&&... args)
{
    assert(get_scheduler());
    get_scheduler_check().go(std::string(name), std::forward<Callable>(fn), std::forward<Args>(args)...);
}

template<typename Callable, typename... Args>
void go(Callable&& fn, Args&&... args)
{
    get_scheduler_check().go(std::forward<Callable>(fn), std::forward<Args>(args)...);
}

// create channel
template<typename T>
channel_pair<T> make_channel(std::size_t capacity)
{
    return get_scheduler_check().make_channel<T>(capacity);
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


// blocks code framgment, exception-safe
template<typename Callable>
void block(Callable callable)
{
    block();
    try
    {
        callable();
        unblock();
    }
    catch(...)
    {
        unblock();
        throw;
    }
}

}

#endif
