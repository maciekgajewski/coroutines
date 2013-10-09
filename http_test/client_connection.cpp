// Copyright (c) 2013 Maciej Gajewski
#include "client_connection.hpp"
#include "coroutines_io/socket_streambuf.hpp"

#include <iostream>
#include <array>

#include "Poco/Net/NetException.h"

client_connection::client_connection(tcp_socket&& s, handler_type&& handler)
    : _socket(std::move(s))
    , _handler(std::move(handler))
{
}


void client_connection::start()
{
    std::cout << "conenction from: " << _socket.remote_endpoint() << std::endl;

    try
    {
        socket_istreambuf ibuf(_socket);
        socket_ostreambuf obuf(_socket);
        std::istream istream(&ibuf);
        std::ostream ostream(&obuf);

        for(;;)
        {
            http_request request(istream);
            try
            {
                request.read_header();
            }
            catch(const Poco::Net::NoMessageException&)
            {
                std::cout << "conenction closed" << std::endl;
                return;
            }
            http_response response(ostream);
            //response.setVersion(http_response::HTTP_1_1);

            _handler(request, response);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "exception in client: " << e.what() << std::endl;
    }
}
