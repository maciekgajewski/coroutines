// Copyright (c) 2013 Maciej Gajewski

#include "tcp_socket.hpp"
#include "service.hpp"
#include "globals.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstring>
#include <system_error>

namespace coroutines {

tcp_socket::tcp_socket(coroutines::service& srv)
    : base_pollable(srv)
{
}

tcp_socket::tcp_socket()
    : base_pollable(get_service_check())
{
}

tcp_socket::tcp_socket(tcp_socket&& o)
    : base_pollable(std::move(o))
    , _remote_endpoint(std::move(o._remote_endpoint))
{
}

tcp_socket::tcp_socket(service& srv, int fd, const endpoint_type& remote_endpoint)
    : base_pollable(srv)
    , _remote_endpoint(remote_endpoint)
{
    set_fd(fd);
}


void tcp_socket::connect(const tcp_socket::endpoint_type& endpoint)
{

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
        cr = ::connect(get_fd(), (sockaddr*)&addr, sizeof(addr));
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
    wait_for_writable();
    _remote_endpoint = endpoint;
}

void tcp_socket::shutdown()
{
    int r = ::shutdown(get_fd(), SHUT_RDWR);
    if (r < 0)
        throw_errno("shutdown");
}


void tcp_socket::open(int address_family)
{
    int fd = ::socket(
        address_family,
        SOCK_STREAM | SOCK_NONBLOCK,
        0);

    if (fd == -1)
    {
        throw_errno();
    }
    else
    {
        set_fd(fd);
    }
}



}
