// Copyright (c) 2013 Maciej Gajewski

#ifndef COROUTINES_TCP_RESOLVER_HPP
#define COROUTINES_TCP_RESOLVER_HPP

#include "coroutines_io/globals.hpp"
#include "coroutines_io/tcp_socket.hpp"

#include <boost/asio/ip/tcp.hpp>

namespace coroutines {

void tcp_resolve(const std::string& hostname, const std::string& service, std::vector<tcp_socket::endpoint_type>& out);

}

#endif
