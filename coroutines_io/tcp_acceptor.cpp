// Copyright (c) 2013 Maciej Gajewski
#include "coroutines_io/tcp_acceptor.hpp"
#include "coroutines_io/globals.hpp"
#include "coroutines_io/io_scheduler.hpp"

#include <sys/socket.h>
#include <netinet/in.h>

#include <iostream>

namespace coroutines {

tcp_acceptor::tcp_acceptor(io_scheduler& srv)
    : base_pollable(srv)
{
}

tcp_acceptor::tcp_acceptor()
    : base_pollable(get_io_scheduler_check())
{
}

void tcp_acceptor::listen(const tcp_acceptor::endpoint_type& endpoint)
{
    assert(!_listening);

    int af = endpoint.address().is_v4() ? AF_INET : AF_INET6;

    open(af);

    int cr = 0;
    if (af == AF_INET)
    {
        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));

        addr.sin_family = af;
        addr.sin_addr.s_addr = htonl(endpoint.address().to_v4().to_ulong());
        addr.sin_port = htons(endpoint.port());
        cr = ::bind(get_fd(), (sockaddr*)&addr, sizeof(addr));
    }
    else
    {
        assert(true); // not implemented
    }

    if (cr < 0)
    {
        throw_errno();
    }

    cr = ::listen(get_fd(), 256); // compeltely arbitrary queue size
    if (cr < 0)
        throw_errno();

    _listening = true;

}

tcp_socket tcp_acceptor::accept()
{
    assert(_listening);

    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    for(;;)
    {
        int fd = ::accept4(get_fd(), (sockaddr*)&addr, &addr_len, SOCK_NONBLOCK);
        if (fd < 0 )
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
//                std::cout << "ACCEPT: acceptor would block" << std::endl;
                wait_for_readable();
                continue;
            }
        }
        else
        {
            boost::asio::ip::address_v4 a(ntohl(addr.sin_addr.s_addr));

            return tcp_socket(get_service(), fd, endpoint_type(a, ntohs(addr.sin_port)));
        }
    };
}

void tcp_acceptor::open(int address_family)
{
    if (!is_open())
    {
        int fd = ::socket(
            address_family,
            SOCK_STREAM | SOCK_NONBLOCK,
            0);

        int one = 1;
        ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

        if (fd == -1)
        {
            throw_errno("open acceptor");
        }
        else
        {
            set_fd(fd);
        }
    }
}

}
