// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_TCP_ACCEPTOR_HPP
#define COROUTINES_TCP_ACCEPTOR_HPP

#include "coroutines/channel.hpp"

#include "coroutines_io/tcp_socket.hpp"

#include <boost/asio/ip/tcp.hpp>

namespace coroutines {

class service;

class tcp_acceptor : public base_pollable
{
public:
    typedef boost::asio::ip::tcp::endpoint endpoint_type;

    tcp_acceptor(coroutines::service& srv);
    tcp_acceptor(); // uses get_service_check()
    tcp_acceptor(const tcp_acceptor&) = delete;

    ~tcp_acceptor() = default;

    void listen(const endpoint_type& endpoint);

    tcp_socket accept();

private:

    void open(int af);

    bool _listening = false;
};

}

#endif
