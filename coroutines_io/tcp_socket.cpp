// Copyright (c) 2013 Maciej Gajewski
#include "tcp_socket.hpp"

namespace coroutines_io {

tcp_socket::tcp_socket(coroutines_io::service& srv)
    : _service(srv)
    , _socket(srv.get_io_service())
{
}

void tcp_socket::connect(const tcp_socket::endpoint_type& endpoint)
{
    // TODO
}



}
