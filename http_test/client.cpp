#include "coroutines_http/http_client.hpp"

#include "coroutines_io/globals.hpp"
#include "coroutines_io/service.hpp"

#include "coroutines/scheduler.hpp"
#include "coroutines/globals.hpp"

#include <iostream>

using namespace coroutines;

void client(const char* url)
{
    try
    {
        http_client client;
        network::http::request req(url);
        network::http::response resp = client.get(req);

        std::string body;
        resp.get_body(body);
        std::cout << body << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << " Error: " << e.what() << std::endl;
    }
}

int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        std::cerr << "USAGE: client URL" << std::endl;
        return 2;
    }

    scheduler sched;
    service serv(sched);
    set_scheduler(&sched);
    set_service(&serv);

    serv.start();

    go(client, argv[1]);

    sched.wait();
}
