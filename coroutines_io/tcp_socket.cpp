// Copyright (c) 2013 Maciej Gajewski

#include "tcp_socket.hpp"
#include "service.hpp"
#include "globals.hpp"

#include <sys/socket.h>
#include <netinet/in.h>

#include <cstring>
#include <system_error>

namespace coroutines {

tcp_socket::tcp_socket(coroutines::service& srv)
    : _service(srv)
{
}

tcp_socket::tcp_socket()
    : _service(get_service_check())
{
}

tcp_socket::~tcp_socket()
{
    close();
}

tcp_socket::tcp_socket(tcp_socket&& o)
    : _service(o._service)
{
    std::swap(_socket, o._socket);
    std::swap(_reader, o._reader);
    std::swap(_writer, o._writer);
}

tcp_socket::tcp_socket(service& srv, int fd)
    : _service(srv)
    , _socket(fd)
{
    auto pair = _service.get_scheduler().make_channel<std::error_code>(1);
    _reader = std::move(pair.reader);
    _writer = std::move(pair.writer);
}


void tcp_socket::connect(const tcp_socket::endpoint_type& endpoint)
{
    auto pair = _service.get_scheduler().make_channel<std::error_code>(1);
    _reader = std::move(pair.reader);
    _writer = std::move(pair.writer);

    int af = endpoint.address().is_v4() ? AF_INET : AF_INET6;

    open(af);

    int cr = 0;
    if (af == AF_INET)
    {
        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));

        addr.sin_family = af;
        addr.sin_addr.s_addr = ::htonl(endpoint.address().to_v4().to_ulong());
        addr.sin_port = ::htons(endpoint.port());
        cr = ::connect(_socket, (sockaddr*)&addr, sizeof(addr));
    }
    else
    {
        assert(true); // not implemented
    }

    if (cr == 0 )
    {
        assert(false);
    }
    if (errno != EINPROGRESS)
    {
        throw_errno("connect");
    }
    _service.wait_for_writable(_socket, _writer);
    std::error_code e = _reader.get();
    if (e)
    {
        throw std::system_error(e, "connect");
    }
}

void tcp_socket::close()
{
    if (_socket != -1)
    {
        ::close(_socket);
        _socket = -1;
    }
}

std::size_t tcp_socket::read(char* buf, std::size_t how_much)
{
     assert(_socket != -1);

     std::size_t total = 0;
     while(total < how_much)
     {
        ssize_t r = ::read(_socket, buf + total, how_much - total);
        if (r == 0)
        {
            return total;
        }
        else if (r < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::cout << "SOCK: waiting for readability of fd=" << _socket << std::endl;
                _service.wait_for_readable(_socket, _writer);
                std::error_code e = _reader.get();
                if (e)
                {
                    throw std::system_error(e, "read");
                }
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

std::size_t tcp_socket::write(const char* buf, std::size_t how_much)
{
     assert(_socket != -1);

     std::size_t total = 0;
     while(total < how_much)
     {
        ssize_t r = ::write(_socket, buf + total, how_much - total);
        if (r == 0)
        {
            return total;
        }
        else if (r < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                _service.wait_for_writable(_socket, _writer);
                std::error_code e = _reader.get();
                if (e)
                {
                    throw std::system_error(e, "write");
                }
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

void tcp_socket::open(int address_family)
{
    _socket = ::socket(
        address_family,
        SOCK_STREAM | SOCK_NONBLOCK,
        0);

    if (_socket == -1)
    {
        throw_errno();
    }
}



}
