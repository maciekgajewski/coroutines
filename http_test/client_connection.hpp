// Copyright (c) 2013 Maciej Gajewski

#ifndef CLIENT_CONNECTION_HPP
#define CLIENT_CONNECTION_HPP

#include "coroutines_io/tcp_socket.hpp"

#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>

using namespace coroutines;

class client_connection
{
public:

    typedef Poco::Net::HTTPRequest http_request;
    typedef Poco::Net::HTTPResponse http_response;

    typedef std::function<void(http_request const&, http_response&)> handler_type;

    client_connection(tcp_socket&& s, handler_type&& handler);

    void start();

private:

    tcp_socket _socket;
    handler_type _handler;
};



#endif
