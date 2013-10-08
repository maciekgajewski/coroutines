// Copyright (c) 2013 Maciej Gajewski
#include "coroutines/globals.hpp"
#include "coroutines/scheduler.hpp"

#include "coroutines_io/globals.hpp"
#include "coroutines_io/service.hpp"
#include "coroutines_io/tcp_acceptor.hpp"

#include <iostream>

using namespace coroutines;
using namespace boost::asio::ip;

void client_connection(tcp_socket& sock)
{
    // TODO
    std::cout << "client conencted" << std::endl;
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
            go("client connection", client_connection, std::move(sock));
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
