// Copyright (c) 2013 Maciej Gajewski

#ifndef COROUTINES_HTTP_CLIENT_HPP
#define COROUTINES_HTTP_CLIENT_HPP

#include "coroutines_io/globals.hpp"

#include <network/http/request.hpp>
#include <network/http/response.hpp>

namespace coroutines {

class http_client
{
public:

    http_client(service& srv) : _service(srv) {}
    http_client() : _service(get_service_check()) {}

    network::http::response get(const network::http::request& request);

private:

    service& _service;
};

} // namespace coroutines

#endif // COROUTINES_HTTP_CLIENT_HPP
