// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "coroutines/thread_pool.hpp"

#include <cassert>

namespace coroutines {

void thread::routine(std::function<void()>& callable)
{
    callable();

    _mutex.lock();
    _completed = true;
    _mutex.unlock();
}

void thread::join()
{
    if (_served_by_pool)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [this]() { return !_completed; });
        _joined = true;
    }
    else
    {
        _thread.join();
    }
}

void detail::parked_thread::routine()
{
    for(;;)
    {
        {
            std::unique_lock<std::mutex> lock(_tp_mutex);

            _fn = nullptr;
            _running = false;
            _cv.wait(lock, [this]() { return _fn == nullptr && !_stopped; } );
            _running = true;
        }

        if (_stopped)
            return;

        assert(_fn);

        _fn();
    }
}

void detail::parked_thread::run(std::function<void ()>&& fn)
{
    std::unique_lock<std::mutex> lock(_tp_mutex);
    assert(!_stopped);
    assert(!_running);

    _fn = std::move(fn);
    _cv.notify_all();
}


void detail::parked_thread::stop_and_join()
{
    {
        std::unique_lock<std::mutex> lock(_tp_mutex);
        _stopped = true;
        _cv.notify_all();
    }
    _thread.join();
}


thread_pool::thread_pool(unsigned size)
{
    assert(size >= 1);
    _threads.reserve(size);
    for(unsigned i = 0; i < size; i++)
    {
        _threads.emplace_back(_mutex);
    }
}

thread_pool::~thread_pool()
{
    for(detail::parked_thread& parked : _threads)
    {
        parked.stop_and_join();
    }
}

bool thread_pool::acquire(std::function<void()>&& fn)
{
    std::lock_guard<std::mutex> lock(_mutex);

    for(detail::parked_thread& parked : _threads)
    {
        if (!parked.running())
        {
            parked.run(std::move(fn));
            return true;
        }
    }

    return false;
}

}
