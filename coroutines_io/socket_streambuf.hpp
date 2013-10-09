// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_IO_SOCKET_STREAMBUF_HPP
#define COROUTINES_IO_SOCKET_STREAMBUF_HPP

#include "coroutines_io/base_pollable.hpp"

#include <array>
#include <streambuf>

namespace coroutines {

class socket_streambuf : public std::streambuf
{
public:

    socket_streambuf(base_pollable& sock)
    : _socket(sock)
    {
    }

    socket_streambuf(const socket_streambuf&) = delete;

private:

    static constexpr unsigned BUFFER_SIZE = 4096;

    std::array<char, BUFFER_SIZE> _buffer;
    base_pollable& _socket;
};



}

#endif
