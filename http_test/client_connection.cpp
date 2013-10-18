// Copyright (c) 2013 Maciej Gajewski
#include "client_connection.hpp"
#include "coroutines_io/socket_streambuf.hpp"

#include <iostream>
#include <array>
#include <algorithm>

#include "Poco/Net/NetException.h"

client_connection::client_connection(tcp_socket&& s, handler_type&& handler)
    : _socket(std::move(s))
    , _handler(std::move(handler))
{
}

// case-insensitive comparison
template<typename StringA, typename StringB>
bool ci_equal(const StringA& a, const StringB& b)
{
    return std::equal(
        std::begin(a), std::end(a), std::begin(b),
        [](char ac, char bc) { return (ac & 0x1f) == (bc & 0x1f); }
        );
}

void client_connection::start()
{
    //std::cout << "conenction from: " << _socket.remote_endpoint() << std::endl;

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
                return;
            }

            http_response response(ostream);

            // honour HTTP 1.0 vs 1.1 and Connection: close
            bool keep_alive = false;
            if (request.getVersion() == http_response::HTTP_1_1 && ci_equal(request.get("Connection", ""), "close"))
            {
                keep_alive = true;
            }
            else if (ci_equal(request.get("Connection", ""), "keep-alive"))
            {
                keep_alive = true;
            }

            // set Date. This is ugly, this should be one-lines with std::put_time
            std::time_t now = std::time(nullptr);
            char date_buffer[64];
            std::strftime(date_buffer, 64, "%a, %d %b %Y %T GMT", std::gmtime(&now));
            response.add("Date", date_buffer);

            // set keep-alive
            if (keep_alive)
            {
                response.add("Connection", "Keep-Alive");
            }

            _handler(request, response);

            if (!keep_alive)
                break;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "exception in client: " << e.what() << std::endl;
    }
}
