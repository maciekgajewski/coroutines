// Copyright (c) 2013 Maciej Gajewski

#include "coroutines_http/http_client.hpp"

#include "coroutines_io/tcp_resolver.hpp"
#include "coroutines_io/tcp_socket.hpp"

#include <boost/lexical_cast.hpp>

#include <boost/asio.hpp>

#include <iostream>

namespace coroutines {


static const std::string HTTP_HEADER_DELIMITER = "\r\n\r\n";

network::http::response http_client::get(const network::http::request& request)
{
    //request.get_destination(
    network::uri uri;
    request.get_uri(uri);

    auto host = uri.host();
    if (!host)
    {
        throw std::runtime_error("URI doesn't specify host");
    }

    // resolve address
    std::vector<tcp_socket::endpoint_type> endpoints;
    tcp_resolve(std::string(*host), "http", endpoints);

    if (endpoints.empty())
    {
        throw std::runtime_error("host not found");
    }

    tcp_socket::endpoint_type endpoint = endpoints.front();
    auto port = uri.port();
    if (port)
    {
        endpoint.port(boost::lexical_cast<std::uint16_t>(*port));
    }

    tcp_socket socket(_service);
    socket.connect(endpoint);

    std::cout << "Connected!!!" << std::endl;

    //read header
    //socket.read_until(buf, size, HTTP_HEADER_DELIMITER);

    // TODO
    return network::http::response();
}

} // namespace coroutines
