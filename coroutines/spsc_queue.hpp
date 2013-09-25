// Copyright (c) 2013 Maciej Gajewski

#ifndef COROUTINES_QUEUE_H
#define COROUTINES_QUEUE_H

#include <vector>
#include <atomic>

#include <cstdlib>
#include <cassert>

namespace coroutines {

// lock-free, fixed size single-producer single-consumer queue.
// Based on https://github.com/facebook/folly/blob/master/folly/ProducerConsumerQueue.h
template<typename T>
class spsc_queue
{
public:

    // capacity must be at least 2, 1 item is always used as divider and not really usable
    spsc_queue(std::size_t capacity);

    spsc_queue(const spsc_queue&) = delete;

    ~spsc_queue();

    // returns true if itemns was moved into queue, false if queue was full
    bool put(T& v);

    // returns false is queue was empty, true if the item was filled with item from queue
    bool get(T& v);

    bool empty() const;
    bool full() const;
    std::size_t size() const; // approx. size

    std::size_t capacity() const { return _capacity; }

private:


    const std::size_t _capacity;
    T* const _data;
    std::atomic<int> _rd;
    std::atomic<int> _wr;
};

template<typename T>
spsc_queue<T>::spsc_queue(std::size_t capacity)
    : _capacity(capacity)
    , _data(static_cast<T*>(std::malloc(sizeof(T) * capacity)))
    , _rd(0)
    , _wr(0)
{
    assert(capacity >= 2);
    if (!_data)
    {
        throw std::bad_alloc();
    }
}

template<typename T>
spsc_queue<T>::~spsc_queue()
{
    // destroy anything that could still be in there
    int rd = _rd;
    int wr = _wr;
    while(rd != wr)
    {
        _data[rd].~T();
        rd = (rd+1) % _capacity;
    }
}

template<typename T>
bool spsc_queue<T>::empty() const
{
    return _rd.load(std::memory_order_consume) = _wr.load(std::memory_order_consume);
}

template<typename T>
bool spsc_queue<T>::full() const
{
    auto wr_next = _wr.load(std::memory_order_consume) + 1;
    if (wr_next == _capacity)
        wr_next = 0;
    return wr_next == _rd.load(std::memory_order_consume);
}

template<typename T>
bool spsc_queue<T>::put(T& v)
{
    int wr_now = _wr.load(std::memory_order_relaxed);

    int wr_next = wr_now + 1;
    if (wr_next == _capacity)
        wr_next = 0;

    if (wr_next != _rd.load(std::memory_order_acquire))
    {
        new(&_data[wr_now]) T(std::move(v));
        _wr.store(wr_next, std::memory_order_release);

        return true;
    }
    return false;
}

template<typename T>
bool spsc_queue<T>::get(T& b)
{
    int rd_now = _rd.load(std::memory_order_relaxed);

    if (rd_now != _wr.load(std::memory_order_acquire))
    {
        int rd_next = rd_now + 1;
        if (rd_next == _capacity)
            rd_next = 0;

        b = std::move(_data[rd_now]);
        _data[rd_now].~T();
        _rd.store(rd_next, std::memory_order_release);

        return true;
    }

    return false;
}

template<typename T>
std::size_t spsc_queue<T>::size() const
{
    int s = _wr.load(std::memory_order_consume) - _rd.load(std::memory_order_consume);
    if (s < 0 )
        s+= _capacity;
    return s;
}

}

#endif
