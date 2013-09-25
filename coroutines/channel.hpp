// Copyright (c) 2013 Maciej Gajewski

#ifndef COROUTINES_CHANNEL_HPP
#define COROUTINES_CHANNEL_HPP

#include <stdexcept>
#include <memory>
#include <utility>

namespace coroutines {

// Exception thrown when channel is closed
struct channel_closed : public std::exception
{
    virtual const char* what() const noexcept { return "channel closed"; }
};

// wrtier implementation interface
template<typename T>
class i_writer_impl
{
public:
    virtual ~i_writer_impl() = default;

    virtual void put(T val) = 0;
    virtual void writer_close() = 0;
};

// wrtier enppoint to a channel
template<typename T>
class channel_writer
{
public:

    channel_writer() noexcept = default;
    channel_writer(channel_writer&& o) noexcept
    {
        std::swap(o._impl, _impl);
    }

    channel_writer(std::shared_ptr<i_writer_impl<T>>&& impl) noexcept
        : _impl(std::move(impl))
    { }

    ~channel_writer()
    {
        if (_impl)
            _impl->writer_close();
    }

    void put(T val)
    {
        if (_impl)
            _impl->put(std::move(val));
        else
            throw channel_closed();
    }

    void close()
    {
        if (_impl)
            _impl->writer_close();
        _impl.reset();
    }

    bool is_closed() const noexcept
    {
        return !_impl;
    }

private:

    std::shared_ptr<i_writer_impl<T>> _impl;
};

//reader implementation interface
template<typename T>
class i_reader_impl
{
public:

    virtual ~i_reader_impl() = default;

    virtual T get() = 0;
    virtual void reader_close() = 0;
};

template<typename T>
class channel_reader
{
public:

    channel_reader() noexcept = default;
    channel_reader(channel_reader&& o)
    {
        std::swap(o._impl, _impl);
    }

    channel_reader(std::shared_ptr<i_reader_impl<T>>&& impl)
    : _impl(std::move(impl))
    { }

    ~channel_reader()
    {
        if (_impl)
            _impl->reader_close();
    }


    T get()
    {
        if (_impl)
            return _impl->get();
        else
            throw channel_closed();
    }

    void close()
    {
        if (_impl)
            _impl->reader_close();
        _impl.reset();
    }

    bool is_closed() const noexcept
    {
        return !_impl;
    }

private:

    std::shared_ptr<i_reader_impl<T>> _impl;
};

template<typename T>
struct channel_pair
{
    channel_pair(channel_pair&& o)
    : reader(std::move(o.reader))
    , writer(std::move(o.writer))
    { }

    channel_pair(channel_reader<T>&& r, channel_writer<T>&& w)
    : reader(std::move(r))
    , writer(std::move(w))
    { }


    channel_reader<T> reader;
    channel_writer<T> writer;
};

}

#endif
