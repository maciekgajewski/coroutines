// Copyright (c) 2013 Maciej Gajewski

#include "coroutines_io/tcp_resolver.hpp"
#include "coroutines_io/io_scheduler.hpp"

#include "coroutines/globals.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <cstring>


namespace coroutines {

void tcp_resolve(const std::string& hostname, const std::string& service, std::vector<tcp_socket::endpoint_type>& out)
{
    addrinfo hints;
    addrinfo* result = nullptr;
    std::memset(&hints, 0, sizeof(addrinfo));

    int r = getaddrinfo(
        hostname.c_str(),
        service.c_str(),
        &hints,
        &result);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (r != 0)
    {
        throw_errno("tcp_resolver::resolve");
    }

    for(addrinfo* addr_info = result; addr_info; addr_info = addr_info->ai_next)
    {
        if(addr_info->ai_family == AF_INET)
        {
            const sockaddr_in* addr = reinterpret_cast<const sockaddr_in*>(addr_info->ai_addr);
            boost::asio::ip::address_v4 a(ntohl(addr->sin_addr.s_addr));

            out.emplace_back(a, ntohs(addr->sin_port));
        }
    }
}

}
