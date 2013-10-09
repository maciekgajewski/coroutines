// Copyright (c) 2013 Maciej Gajewski
#include "client_connection.hpp"
#include "coroutines_io/socket_streambuf.hpp"

#include <iostream>
#include <array>

client_connection::client_connection(tcp_socket&& s, handler_type&& handler)
    : _socket(std::move(s))
    , _handler(std::move(handler))
{
}


void client_connection::start()
{
    std::cout << "conenction from: " << _socket.remote_endpoint() << std::endl;

    socket_istreambuf ibuf(_socket);
    socket_ostreambuf obuf(_socket);
    std::istream istream(&ibuf);
    std::ostream ostream(&obuf);

    http_request request(istream);
    http_response response(ostream);

    _handler(request, response);
}
