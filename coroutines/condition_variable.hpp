// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef CONDITION_VARIABLE_HPP
#define CONDITION_VARIABLE_HPP

#include "monitor.hpp"

namespace coroutines {

// coroutine version of condition variables.
// partially source-compatible with std::consdition_variable_any
class condition_variable
{
public:

    condition_variable(scheduler& sched)
        : _monitor(sched)
    { }

    void notify_all()
    {
        _monitor.wake_all();
    }

    void notify_one()
    {
        _monitor.wake_one();
    }

    // Unlocks the lock and waits in an atomic way.
    template<typename Lock>
    void wait(const std::string& checkpoint_name, Lock& lock);

    template<typename Lock, typename Predicate>
    void wait(const std::string& checkpoint_name, Lock& lock, Predicate pred)
    {
        while(!pred())
            wait(checkpoint_name, lock);
    }

private:

    monitor _monitor;
};


template<typename Lock>
void condition_variable::wait(const std::string& checkpoint_name, Lock& lock)
{
    _monitor.wait(checkpoint_name, [&lock]()
    {
        // this code will bve called after the coroutine yields and its added to monitor
        lock.unlock();
    });
    lock.lock();
}

}

#endif // CONDITION_VARIABLE_HPP
