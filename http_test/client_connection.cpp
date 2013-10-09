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

            // honour HTTP 1.0 vs 1.1 and Connection: close
            bool close_conection = true;
            if (request.getVersion() == http_response::HTTP_1_1)
            {
                response.setVersion(http_response::HTTP_1_1);
                if(request.get("Connection", "") != "close")
                {
                    response.add("Connection", "keep-alive");
                    close_conection = false;
                }
            }

            // set Date. This is ugly, this should be one-lines with std::put_time
            std::time_t now = std::time(nullptr);
            char date_buffer[64];
            std::strftime(date_buffer, 64, "%a, %d %b %Y %T GMT", std::gmtime(&now));
            response.add("Date", date_buffer);

            _handler(request, response);

            if (close_conection)
                break;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "exception in client: " << e.what() << std::endl;
    }
}
