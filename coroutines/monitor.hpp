// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_MONITOR_HPP
#define COROUTINES_MONITOR_HPP

#include "coroutines/coroutine.hpp"
#include "coroutines/mutex.hpp"

#include <vector>
#include <functional>

namespace coroutines {

class scheduler;

// monitor is a syncronisation tool.
// it allows one corotunie to wait for singla from another.
class monitor
{
public:

    typedef std::function<void ()> epilogue_type;

    monitor(scheduler& sched);
    monitor(const monitor&) = delete;
    ~monitor();

    // called from corotunie context. Will cause the corountine to yield
    // Epilogue will be called after the coroutine is preemted
    void wait(const std::string& checkpoint_name, epilogue_type epilogue = epilogue_type());

    // wakes all waiting corotunies
    void wake_all();

    // wakes one of the waiting corountines
    void wake_one();


private:

    std::vector<coroutine_weak_ptr> _waiting;
    mutex _waiting_mutex;

    scheduler& _scheduler;
};

}

#endif
