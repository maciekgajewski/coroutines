// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_IO_TCP_SOCKET_HPP
#define COROUTINES_IO_TCP_SOCKET_HPP

#include "coroutines/channel.hpp"

#include "coroutines_io/base_pollable.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <system_error>

namespace coroutines {

class io_scheduler;


// implements boost:asio::SyncReadStream
class tcp_socket : public base_pollable
{
public:

    typedef boost::asio::ip::tcp::endpoint endpoint_type;

    // then object uses service and must not outlive it
    tcp_socket(io_scheduler& srv);
    tcp_socket(); // uses get_io_scheduler_check()

    tcp_socket(const tcp_socket&) = delete;
    tcp_socket(tcp_socket&&);

    tcp_socket(io_scheduler& srv, int get_fd, const endpoint_type& remote_endpoint);

    ~tcp_socket() = default;

    void connect(const endpoint_type& endpoint);

    endpoint_type remote_endpoint() const { return _remote_endpoint; }

    void shutdown();

private:

    void open(int address_family);
    endpoint_type _remote_endpoint;

};

}

#endif
