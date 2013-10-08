#include "client_connection.hpp"

#include <iostream>

client_connection::client_connection(tcp_socket&& s)
: _socket(std::move(s))
{
}


void client_connection::start()
{
    std::cout << "conenction from: " << _socket.remote_endpoint() << std::endl;
}
