// Copyright (c) 2013 Maciej Gajewski

#include "tcp_socket.hpp"
#include "service.hpp"
#include "globals.hpp"

namespace coroutines {

tcp_socket::tcp_socket(coroutines::service& srv)
    : _service(srv)
    , _socket(srv.get_io_service())
{
}

tcp_socket::tcp_socket()
    : _service(get_service_check())
    , _socket(_service.get_io_service())
{
}

tcp_socket::~tcp_socket()
{
}

void tcp_socket::connect(const tcp_socket::endpoint_type& endpoint)
{
    auto pair = _service.get_scheduler().make_channel<boost::system::error_code>(1);
    channel_writer<boost::system::error_code> writer = std::move(pair.writer);

    _socket.async_connect(
        endpoint,
        [&writer](const boost::system::error_code& error)
        {
            std::cout << "tcp_socket connect handler called, error: " << error << std::endl;
            writer.put(error);
        });

    boost::system::error_code error = pair.reader.get();
    std::cout << "tcp_socket connect status received, error: " << error << std::endl;
    if(error)
    {
        throw boost::system::system_error(error);
    }
}



}
