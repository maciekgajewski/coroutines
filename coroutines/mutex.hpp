// Copyright (c) 2013 Maciej Gajewski

#ifndef COROUTINES_MUTEX_HPP
#define COROUTINES_MUTEX_HPP

#include <mutex>
#include <atomic>

namespace coroutines {


class spinlock
{
public:

    spinlock() 
    : _flag(ATOMIC_FLAG_INIT)
    { }

    void lock()
    {
        while(_flag.test_and_set(std::memory_order_acquire))
            ; // spin
    }

    bool try_lock()
    {
        return !_flag.test_and_set(std::memory_order_acquire);
    }

    void unlock()
    {
        _flag.clear(std::memory_order_release);
    }


private:

    std::atomic_flag _flag;
};


//typedef std::mutex mutex;
typedef spinlock mutex;

}

#endif
