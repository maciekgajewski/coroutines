// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_THREAD_POOL_HPP
#define COROUTINES_THREAD_POOL_HPP

#include "coroutines/mutex.hpp"

#include <vector>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

#include <iostream>
#include <cassert>
#include <atomic>

namespace coroutines {

namespace detail {

class parked_thread
{
public:

    parked_thread()
        : _thread([this]() { routine(); })
    { }

    parked_thread(parked_thread&& o)
        : _fn(std::move(o._fn))
        , _thread(std::move(o._thread))
    {
        std::swap(_stopped, o._stopped);
        std::swap(_running, o._running);
    }

    // if thread is busy, returns false
    bool run(std::function<void ()>&& fn);
    void stop_and_join();
    void join();

private:

    void routine();
    mutex _mutex;
    std::condition_variable_any _cv;
    std::condition_variable_any _join_cv;
    std::function<void()> _fn;
    bool _stopped = false;
    bool _running = false;
    std::thread _thread;
};

class free_thread
{
public:

    template<typename Callable>
    free_thread(Callable&& c)
        : _finished(false)
        , _thread([c, this]()
        {
            c();
            _finished = true;
        })
    { }

    bool finished() const { return _finished; }
    void join() noexcept { _thread.join(); }

private:

    std::atomic<bool> _finished;
    std::thread _thread;
};

typedef std::unique_ptr<free_thread> free_thread_ptr;

}

class thread_pool
{
public:

    thread_pool(unsigned size);
    ~thread_pool();

    void run(std::function<void()> fn);

    // blocks until all threads are done
    void join();

    // blocks until all threads are done, stops parked threads
    void stop_and_join();

private:

    void join_completed();
    void create_free_thread(std::function<void ()> fn);
    void join_all_free_threads();

    std::vector<detail::parked_thread> _parked_threads;
    std::vector<detail::free_thread_ptr> _free_threads;
    std::mutex _free_threads_mutex;
};

}

#endif
