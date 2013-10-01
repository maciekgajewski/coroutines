// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_IO_TCP_SOCKET_HPP
#define COROUTINES_IO_TCP_SOCKET_HPP

#include "buffer.hpp"
#include "coroutines/channel.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <system_error>

namespace coroutines {

class service;

class tcp_socket
{
public:

    typedef boost::asio::ip::tcp::endpoint endpoint_type;

    // then object uses service and must not outlive it
    tcp_socket(service& srv);
    tcp_socket(); // uses get_service_check()

    tcp_socket(const tcp_socket&) = delete;
    tcp_socket(tcp_socket&&);

    tcp_socket(service& srv, int fd);

    ~tcp_socket();

    void connect(const endpoint_type& endpoint);
    void close();

    std::size_t read(char* buf, std::size_t how_much);
    std::size_t write(const char* buf, std::size_t how_much);


private:

    void open(int address_family);

    service& _service;

    int _socket = -1;

    channel_reader<std::error_code> _reader;
    channel_writer<std::error_code> _writer;
};

}

#endif
