// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_NAIVE_CHANNEL_HPP
#define COROUTINES_NAIVE_CHANNEL_HPP

#include <memory>
#include <list>
#include <mutex>
#include <condition_variable>

namespace corountines {

// simple channel
template<typename T>
class naive_channel
{
public:

    void put(const T& v) { _impl->put(v); }
    T get() { return _impl->get(); }

private:

    friend class naive_scheduler;
    naive_channel(std::size_t capacity);

    class impl
    {
    public:
        impl(std::size_t capacity);

        void put(const T& v);
        T get();
    private:
        std::list<T> _queue;
        const std::size_t _capacity;
        std::mutex _mutex;
        std::condition_variable _cv;
    };

    std::shared_ptr<impl> _impl;
};

template<typename T>
naive_channel<T>::naive_channel(std::size_t capacity)
    : _impl(std::make_shared<impl>(capacity))
{
}

template<typename T>
naive_channel<T>::impl::impl(std::size_t capacity)
    : _capacity(capacity)
{
}

template<typename T>
void naive_channel<T>::impl::put(const T& v)
{
    std::unique_lock<std::mutex> lock(_mutex);

    // wait for the queue to be not-full
    _cv.wait(lock, [this](){ return _queue.size() < _capacity; });

    if(_queue.empty())
        _cv.notify_all();
    _queue.push_back(v);
}

template<typename T>
T naive_channel<T>::impl::get()
{
    std::unique_lock<std::mutex> lock(_mutex);

    // wait for the queue to be not-empty
    _cv.wait(lock, [this](){ return !_queue.empty(); });

    T v = _queue.front();
    _queue.pop_front();
    if (_queue.size() == _capacity - 1)
        _cv.notify_all();
    return v;
}


}

#endif
