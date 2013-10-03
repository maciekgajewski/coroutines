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
channel_pair<T> make_channel(std::size_t capacity, const std::string& name = std::string())
{
    return get_scheduler_check().make_channel<T>(capacity, name);
}

// begin blocking operation
// starting coroutines is not allowed in blocking mode
inline void block(const std::string& checkpoint_name = std::string())
{
    context* ctx = context::current_context();
    assert(ctx);
    ctx->block(checkpoint_name);
}

// ends blocking mode. may preempt current coroutine
inline void unblock(const std::string& checkpoint_name = std::string())
{
    context* ctx = context::current_context();
    assert(ctx);
    ctx->unblock(checkpoint_name);
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

template<typename Callable>
void block(const std::string& checkpoint_name, Callable callable)
{
    block(checkpoint_name + " block");
    try
    {
        callable();
        unblock(checkpoint_name + " unblock");
    }
    catch(...)
    {
        unblock(checkpoint_name + " unblock after exception");
        throw;
    }
}

}

#endif
