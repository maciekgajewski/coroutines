// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_NAIVE_CHANNEL_HPP
#define COROUTINES_NAIVE_CHANNEL_HPP

#include <memory>
#include <list>

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

}

#endif
