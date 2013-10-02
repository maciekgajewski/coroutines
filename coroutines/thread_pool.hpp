// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_THREAD_POOL_HPP
#define COROUTINES_THREAD_POOL_HPP

#include <vector>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

#include <iostream>
#include <cassert>

namespace coroutines {

namespace detail {

class parked_thread
{
public:

    parked_thread(std::mutex& m)
        : _tp_mutex(m), _thread([this]() { routine(); })
    { }

    parked_thread(parked_thread&& o)
        : _tp_mutex(o._tp_mutex)
        , _fn(std::move(o._fn))
        , _thread(std::move(o._thread))
    {
        std::swap(_stopped, o._stopped);
        std::swap(_running, o._running);
    }

    void run(std::function<void ()>&& fn); // call with mutex locked
    bool running() const { return _running; } // call with mutex locked
    void stop_and_join();

private:

    void routine();
    std::mutex& _tp_mutex;
    std::condition_variable _cv;
    std::function<void()> _fn;
    bool _stopped = false;
    bool _running = false;
    std::thread _thread;
};

}



class thread_pool
{
public:

    thread_pool(unsigned size);
    ~thread_pool();

private:

    friend class thread;

    bool acquire(std::function<void()>&& fn);

    std::vector<detail::parked_thread> _threads;
    std::mutex _mutex;
};

// thread - should be at least partially compatible with std::thread
class thread
{
public:
    thread(); // not-a-thread
    thread(const thread&) = delete;
    thread(thread&& o);

    template<typename Callable>
    thread(thread_pool& tp, Callable&& fn)
    {
        _served_by_pool = tp.acquire([this, fn]() { routine(fn); } );
        if (!_served_by_pool)
        {
            std::cout << "THREAD this=" << this << " not served by the pool" << std::endl;
            _thread = std::move(std::thread(std::move(fn)));
        }
        else
        {
            std::cout << "THREAD this=" << this << " served by the pool" << std::endl;
        }
    }

    void join();

    bool joinable() const noexcept
    {
        if (_served_by_pool)
            return !_joined;
        else
            return _thread.joinable();
    }

private:

    void routine(std::function<void()> callable);


    bool _served_by_pool = false;
    bool _completed = false;
    bool _joined = false;
    std::mutex _mutex;
    std::condition_variable _cv;

    // fallback thread, if unable to acquirte one form pool
    std::thread _thread;

};

}

#endif
