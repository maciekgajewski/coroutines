// Copyright (c) 2013 Maciej Gajewski
#include "coroutines/globals.hpp"
#include "coroutines/scheduler.hpp"

#include "coroutines_io/globals.hpp"
#include "coroutines_io/service.hpp"
#include "coroutines_io/tcp_acceptor.hpp"

#include "client_connection.hpp"

#include <iostream>
#include <array>

using namespace coroutines;
using namespace boost::asio::ip;

void handler(network::http::request const& req, network::http::response& res)
{
    res.set_status(network::http::response::
    res.append_header("boo", "123");
    res.append_body("honolulu");
}

void start_client_connection(tcp_socket& sock)
{
    std::cout << "client conencted" << std::endl;
    client_connection c(std::move(sock), handler);
    c.start();
}

void server()
{
    try
    {
        tcp_acceptor acc;
        acc.listen(tcp::endpoint(address_v4::any(), 8080));

        for(;;)
        {
            tcp_socket sock = acc.accept();
            go("client connection", start_client_connection, std::move(sock));
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "server error: " << e.what() << std::endl;
    }
}


int main(int argc, char** argv)
{
    scheduler sched;
    service srv(sched);
    set_scheduler(&sched);
    set_service(&srv);

    srv.start();

    go("acceptor", server);


    sched.wait();
}
