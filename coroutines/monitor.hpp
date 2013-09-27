// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_MONITOR_HPP
#define COROUTINES_MONITOR_HPP

#include "coroutine.hpp"
#include "thread_safe_queue.hpp"

namespace coroutines {

// monitor is a syncronisation tool.
// it allows one corotunie to wait for singla from another.
class monitor
{
public:
    monitor();
    monitor(const monitor&) = delete;
    ~monitor();

    // called from corotunie context. Will cause the corountine to yield
    void wait();

    // wakes all waiting corotunies
    void wake_all();

    // wakes one of the waiting corountines
    void wake_one();


private:

    thread_safe_queue<coroutine> _waiting;
};

}

#endif
