// Copyright (c) 2013 Maciej Gajewski

#include "coroutines_io/base_pollable.hpp"

#include "coroutines_io/service.hpp"

#include <unistd.h>

namespace coroutines {

base_pollable::base_pollable(service& srv)
    : _service(srv)
{
}

base_pollable::base_pollable(base_pollable&& o)
    : _service(o._service)
{
    std::swap(_fd, o._fd);
    std::swap(_reader, o._reader);
    std::swap(_writer, o._writer);
}

base_pollable::~base_pollable()
{
    close();
}

void base_pollable::close()
{
    if (_fd != -1)
    {
        ::close(_fd);
        _fd = -1;
    }
}

void base_pollable::set_fd(int fd)
{
    assert(_fd == -1);

    _fd = fd;

    auto pair = _service.get_scheduler().make_channel<std::error_code>(1);
    _reader = std::move(pair.reader);
    _writer = std::move(pair.writer);

}

void base_pollable::wait_for_readable()
{
    assert(_fd != -1);

    _service.wait_for_readable(_fd, _writer);
    std::error_code e = _reader.get();
    if (e)
    {
        throw std::system_error(e, "wait_for_readable");
    }
}

void base_pollable::wait_for_writable()
{
    _service.wait_for_writable(_fd, _writer);
    std::error_code e = _reader.get();
    if (e)
    {
        throw std::system_error(e, "wait_for_writable");
    }

}

std::size_t base_pollable::read(char* buf, std::size_t how_much)
{
     assert(is_open());

     std::size_t total = 0;
     while(total < how_much)
     {
        ssize_t r = ::read(get_fd(), buf + total, how_much - total);
        if (r == 0)
        {
            return total;
        }
        else if (r < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                wait_for_readable();
            }
            else
            {
                throw_errno("read");
            }
        }
        else
        {
            total += r;
        }
     }
     return total;
}

std::size_t base_pollable::write(const char* buf, std::size_t how_much)
{
     assert(is_open());

     std::size_t total = 0;
     while(total < how_much)
     {
        ssize_t r = ::write(get_fd(), buf + total, how_much - total);
        if (r == 0)
        {
            return total;
        }
        else if (r < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                wait_for_writable();
            }
            else
            {
                throw_errno("write");
            }
        }
        else
        {
            total += r;
        }
     }
     return total;
}

}
