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

    // wakes any waiting corotunies
    void wake_all();

    // TODO: is wake_one needed?

private:

    thread_safe_queue<coroutine> _waiting;
};

}

#endif
