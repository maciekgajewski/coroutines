// Copyright (c) 2013 Maciej Gajewski
//
// Based on cpp-netlib sycn client:
// Copyright 2009 (c) Dean Michael Berris <dberris@google.com>
// Copyright 2009 (c) Tarroo, Inc.
// Adapted from Christopher Kholhoff's Boost.Asio Example, released under
// the Boost Software License, Version 1.0. (See acccompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CLIENT_CONNECTION_HPP
#define CLIENT_CONNECTION_HPP

#include "coroutines_io/tcp_socket.hpp"

#include "boost/network/protocol/http/request.hpp"
#include "boost/network/protocol/http/response.hpp"
#include "boost/network/protocol/http/server/request_parser.hpp"

#include <array>

using namespace coroutines;
namespace net = boost::network;

class client_connection
{
public:

    typedef std::function<void(net::http::request const&, net::http::response&)> handler_type;

    client_connection(tcp_socket&& s, handler_type&& handler);

    void start();

private:
    enum state_t {
        method,
        uri,
        version,
        headers,
        body
    };

    void client_error();
    void flatten_response();
    void segmented_write(std::string data);

    tcp_socket _socket;
    handler_type _handler;

    typedef std::array<char,4096> buffer_type;
    net::http::request_parser parser_;
    net::http::request request_;
    net::http::response response_;
    std::list<buffer_type> output_buffers_;
    std::string partial_parsed;
    boost::optional<boost::system::system_error> error_encountered;
    bool read_body_;
};



#endif
