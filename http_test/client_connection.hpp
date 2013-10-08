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

#include <network/http/request.hpp>
#include <network/http/response.hpp>
#include <network/protocol/http/server/request_parser.hpp>

#include <array>

using namespace coroutines;

class client_connection
{
public:

    typedef std::function<void(network::http::request const&, network::http::response&)> handler_type;

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
    network::http::request_parser parser_;
    network::http::request request_;
    network::http::response response_;
    std::list<buffer_type> output_buffers_;
    std::string partial_parsed;
    boost::optional<boost::system::system_error> error_encountered;
    bool read_body_;
};



#endif
