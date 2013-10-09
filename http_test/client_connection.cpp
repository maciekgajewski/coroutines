#include "client_connection.hpp"

#include <iostream>

client_connection::client_connection(tcp_socket&& s, handler_type&& handler)
    : _socket(std::move(s))
    , _handler(std::move(handler))
{
}


void client_connection::start()
{
    std::cout << "conenction from: " << _socket.remote_endpoint() << std::endl;

}
