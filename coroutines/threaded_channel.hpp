// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_NAIVE_CHANNEL_HPP
#define COROUTINES_NAIVE_CHANNEL_HPP

#include <memory>
#include <vector>
#include <condition_variable>
#include <cassert>
#include <utility>
#include <iostream> // for debugging

#include "channel.hpp"
#include "mutex.hpp"
#include "spsc_queue.hpp"

namespace coroutines {


// simple channel
template<typename T>
class threaded_channel : public i_writer_impl<T>, public i_reader_impl<T>
{
public:

    // factory
    static channel_pair<T> make(std::size_t capacity)
    {
        std::shared_ptr<threaded_channel<T>> me(new threaded_channel<T>(capacity));
        return channel_pair<T>(channel_reader<T>(me), channel_writer<T>(me));
    }

    threaded_channel(const threaded_channel&) = delete;

    // called by writer
    virtual void put(T v) override;

    // caled by reader
    virtual T get() override;

    // caled by both
    virtual void close() override;

private:

    threaded_channel(std::size_t capacity);


    spsc_queue<T> _queue;

    mutex _read_mutex, _write_mutex;
    std::condition_variable_any _cv;

    bool _closed = false;
};

template<typename T>
threaded_channel<T>::threaded_channel(std::size_t capacity)
    : _queue(capacity+1)
{
}


template<typename T>
void threaded_channel<T>::put(T v)
{
    std::lock_guard<mutex> lock(_write_mutex);

    if (_closed)
        return;

    // try to insert without locking
    if (!_queue.put(v))
    {
        // failed, locking (and possiibly waiting) needed
        std::lock_guard<mutex> lock(_read_mutex);
        _cv.wait(_read_mutex, [this, &v](){ return  _queue.put(v) || _closed; });
    }

    if(_queue.size() == 1)
        _cv.notify_all();
}

template<typename T>
T threaded_channel<T>::get()
{
    std::lock_guard<mutex> lock(_read_mutex);
    T result;
    bool success = true;

    // try to read wihtout blocking
    if(!_queue.get(result))
    {
        // failed, locking (and possiibly waiting) needed
        std::lock_guard<mutex> lock(_write_mutex);
        // wait for the queue to be filled
        _cv.wait(_write_mutex, [this, &result, &success](){ return (success = _queue.get(result)) || _closed; });
    }

    if (!success)
    {
        assert(_closed);
        throw channel_closed();
    }

    if (_queue.size() == _queue.capacity() - 1)
        _cv.notify_all();
    return result;
}

template<typename T>
void threaded_channel<T>::close()
{
    std::lock(_read_mutex, _write_mutex);

    if (!_closed)
    {
        _closed = true;
        _cv.notify_all();
    }
    _read_mutex.unlock();
    _write_mutex.unlock();

}

}

#endif
