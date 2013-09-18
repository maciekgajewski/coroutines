// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_NAIVE_CHANNEL_HPP
#define COROUTINES_NAIVE_CHANNEL_HPP

#include <memory>
#include <vector>
#include <condition_variable>
#include <cassert>
#include <utility>

#include "channel.hpp"
#include "mutex.hpp"

namespace coroutines {


// simple channel
template<typename T>
class threaded_channel
{
public:

    // factory
    static channel_pair<T> make(std::size_t capacity)
    {
        std::shared_ptr<threaded_channel<T>> me(new threaded_channel<T>(capacity));
        return channel_pair<T>({
            channel_reader<T>(std::unique_ptr<i_reader_impl<T>>(new reader_impl(me))),
            channel_writer<T>(std::unique_ptr<i_writer_impl<T>>(new writer_impl(me)))}
            );
    }

    threaded_channel(const threaded_channel&) = delete;

    // called by writer
    void put(const T& v);
    void put(T&& v);
    void close();

    // caled by reader
    T get();

private:

    threaded_channel(std::size_t capacity);

    class reader_impl : public i_reader_impl<T>
    {
    public:
        virtual ~reader_impl() noexcept override  = default;
        virtual T get() override { return _parent->get(); }

    private:

        reader_impl() = delete;
        reader_impl(const reader_impl&) = delete;
        reader_impl(std::shared_ptr<threaded_channel> parent) : _parent(parent) { }
        std::shared_ptr<threaded_channel> _parent;

        friend class threaded_channel<T>;
    };

    class writer_impl : public i_writer_impl<T>
    {
    public:
        virtual ~writer_impl() noexcept override { _parent->close(); }
        virtual void put(const T& v) override { _parent->put(v); }
        virtual void put(T&& v) override { _parent->put(v); }
        virtual void close() override { _parent->close(); }

    private:

        writer_impl() = delete;
        writer_impl(const writer_impl&) = delete;
        writer_impl(std::shared_ptr<threaded_channel> parent) : _parent(parent) { }
        std::shared_ptr<threaded_channel> _parent;

        friend class threaded_channel<T>;
    };

    std::size_t size() const noexcept
    {
        return (_wr - _rd + _capacity) % _capacity;
    }

    bool empty() const noexcept
    {
        return _wr == _rd;
    }

    std::vector<T> _queue;
    const std::size_t _capacity;
    std::size_t _rd = 0; // index of the next item to read
    std::size_t _wr = 0; // index of nex item to write
    mutex _mutex;
    std::condition_variable _cv;
    bool _closed = false;
};

template<typename T>
threaded_channel<T>::threaded_channel(std::size_t capacity)
    : _capacity(capacity+1)
{
    assert(capacity > 0);
    _queue.reserve(_capacity);
}


template<typename T>
void threaded_channel<T>::put(const T& v)
{
    std::unique_lock<mutex> lock(_mutex);

    // wait for the queue to be not-full
    _cv.wait(lock, [this](){ return size() < _capacity-1; });

    _queue[_wr] = v;
    _wr = (_wr + 1) % _capacity;
    if(size() == 1)
        _cv.notify_all();
}

template<typename T>
T threaded_channel<T>::get()
{
    std::unique_lock<mutex> lock(_mutex);

    // wait for the queue to be not-empty
    _cv.wait(lock, [this](){ return size() > 0 || _closed; });

    if (_closed)
        throw channel_closed();

    T v = _queue[_rd];
    _rd = (_rd + 1) % _capacity;
    if (size() == _capacity - 2)
        _cv.notify_all();
    return v;
}

template<typename T>
void threaded_channel<T>::close()
{
    std::unique_lock<mutex> lock(_mutex);

    if (!_closed)
    {
        _closed = false;
        if (empty()) // someone may be waiting
        {
            _cv.notify_all();
        }
    }
}

}

#endif
