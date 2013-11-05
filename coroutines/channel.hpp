// Copyright (c) 2013 Maciej Gajewski

#ifndef COROUTINES_CHANNEL_HPP
#define COROUTINES_CHANNEL_HPP

#include "coroutines/locking_channel.hpp"

#include <memory>
#include <utility>

namespace coroutines {


// writer enppoint to a channel
template<typename T, typename implementation_type=typename locking_channel<T>::writer>
class channel_writer
{
public:

    channel_writer() noexcept = default;
    channel_writer(const channel_writer&) = default;
    channel_writer(channel_writer&& o) noexcept
    {
        std::swap(o._impl, _impl);
    }

    channel_writer(std::shared_ptr<implementation_type>&& impl) noexcept
        : _impl(std::move(impl))
    { }

    channel_writer(const std::shared_ptr<implementation_type>& impl) noexcept
        : _impl(impl)
    { }

    ~channel_writer()
    {
    }

    channel_writer<T, implementation_type>& operator=(channel_writer&& o)
    {
        std::swap(o._impl, _impl);
        return *this;
    }

    void put(T val)
    {
        if (_impl)
            _impl->put(std::move(val));
        else
            throw channel_closed();
    }

    // does not throw when channel is closed
    void put_nothrow(T val)
    {
        try
        {
            if (_impl)
                _impl->put(std::move(val));
        }
        catch(const channel_closed&)
        {
        }
    }

    void close()
    {
        _impl.reset();
    }

private:

    std::shared_ptr<implementation_type> _impl;
};

template<typename T, typename implementation_type=typename locking_channel<T>::reader>
class channel_reader
{
public:

    channel_reader() noexcept = default;
    channel_reader(const channel_reader&) = default;
    channel_reader(channel_reader&& o)
    {
        std::swap(o._impl, _impl);
    }

    channel_reader<T, implementation_type>& operator=(channel_reader&& o)
    {
        std::swap(o._impl, _impl);
        return *this;
    }

    channel_reader(std::shared_ptr<implementation_type>&& impl) noexcept
    : _impl(std::move(impl))
    { }

    channel_reader(const std::shared_ptr<implementation_type>& impl)
    : _impl(impl)
    { }

    ~channel_reader()
    {
    }


    T get()
    {
        if (_impl)
            return _impl->get();
        else
            throw channel_closed();
    }

    bool try_get(T& b)
    {
        if (_impl)
            return _impl->try_get(b);
        else
            throw channel_closed();
    }

    void close()
    {
        _impl.reset();
    }

    bool is_closed() const noexcept
    {
        return !_impl;
    }

private:

    std::shared_ptr<implementation_type> _impl;
};

template<typename T, typename channel_type=locking_channel<T>>
struct channel_pair
{
    typedef channel_reader<T, typename channel_type::reader> reader_type;
    typedef channel_writer<T, typename channel_type::writer> writer_type;

    // factory
    static channel_pair<T, channel_type > make(scheduler& sched, std::size_t capacity, const std::string& name)
    {
        std::shared_ptr<channel_type> channel(std::make_shared<channel_type>(sched, capacity, name));
        std::shared_ptr<typename channel_type::writer> writer(std::make_shared<typename channel_type::writer>(channel));
        std::shared_ptr<typename channel_type::reader> reader(std::make_shared<typename channel_type::reader>(channel));

        return channel_pair<T, channel_type>(reader_type(reader), writer_type(writer));
    }

    channel_pair(const channel_pair& o) = default;

    channel_pair(channel_pair&& o)
    : reader(std::move(o.reader))
    , writer(std::move(o.writer))
    { }

    channel_pair(reader_type&& r, writer_type&& w)
    : reader(std::move(r))
    , writer(std::move(w))
    { }

    reader_type reader;
    writer_type writer;
};

}

#endif
