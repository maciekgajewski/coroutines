// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_IO_TCP_SOCKET_HPP
#define COROUTINES_IO_TCP_SOCKET_HPP

#include "buffer.hpp"

#include <boost/asio.hpp>

namespace coroutines_io {

class service;

class tcp_socket
{
public:

    typedef boost::asio::ip::tcp::endpoint endpoint_type;

    // then object uses service and must not outlive it
    tcp_socket(service& srv);
    tcp_socket(); // uses get_service_check()

    ~tcp_socket();

    void connect(const endpoint_type& endpoint);

    buffer read(unsigned how_much);
    void read(buffer& data);
    void write(buffer& data);

    void connect(/* address */);
    void close();

private:

    service& _service;
    boost::asio::ip::tcp::socket _socket;
};

}

#endif
