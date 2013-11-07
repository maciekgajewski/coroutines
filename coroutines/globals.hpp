// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_GLOBALS_HPP
#define COROUTINES_GLOBALS_HPP

#include "coroutines/scheduler.hpp"
#include "coroutines/processor.hpp"

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

}

#endif
