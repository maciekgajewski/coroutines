// Copyright (c) 2013 Maciej Gajewski
#include "coroutines_io/tcp_acceptor.hpp"
#include "coroutines_io/globals.hpp"
#include "coroutines_io/service.hpp"

#include <sys/socket.h>
#include <netinet/in.h>

namespace coroutines {

tcp_acceptor::tcp_acceptor(service& srv)
    : _service(srv)
{
}

tcp_acceptor::tcp_acceptor()
    : _service(get_service_check())
{
}

tcp_acceptor::~tcp_acceptor()
{
    ::close(_socket);
}

void tcp_acceptor::listen(const tcp_acceptor::endpoint_type& endpoint)
{
    assert(!_listening);

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
        cr = ::bind(_socket, (sockaddr*)&addr, sizeof(addr));
    }
    else
    {
        assert(true); // not implemented
    }

    if (cr < 0)
    {
        throw_errno();
    }

    cr = ::listen(_socket, 256);
    if (cr < 0)
        throw_errno();

    _listening = true;

}

tcp_socket tcp_acceptor::accept()
{
    assert(_listening);

    sockaddr addr;
    socklen_t addr_len = sizeof(addr);
    for(;;)
    {
        int fd = ::accept4(_socket, &addr, &addr_len, SOCK_NONBLOCK);
        if (fd < 0 )
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                std::cout << "ACCEPT: acceptor would block" << std::endl;
                _service.wait_for_writable(_socket, _writer);
                std::error_code ec = _reader.get();
                if (ec)
                {
                    throw std::system_error(ec, "what");
                }
                continue;
            }
        }
        else
        {
            return tcp_socket(_service, fd);
        }
    };
}

void tcp_acceptor::open(int address_family)
{
    if (_socket == -1)
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

}
