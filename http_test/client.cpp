#include "coroutines_http/http_client.hpp"

#include "coroutines_io/globals.hpp"
#include "coroutines_io/service.hpp"

#include "coroutines/scheduler.hpp"
#include "coroutines/globals.hpp"

#include <boost/asio.hpp> // TODO remove

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

//    scheduler sched;
//    service serv(sched);
//    set_scheduler(&sched);
//    set_service(&serv);

//    serv.start();

//    go(client, argv[1]);

//    sched.wait();

    using boost::asio::ip::tcp;

    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(argv[1], "http");
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    tcp::socket socket(io_service);
    boost::asio::connect(socket, endpoint_iterator);

    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << "GET " << argv[2] << " HTTP/1.0\r\n";
    request_stream << "Host: " << argv[1] << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    // Send the request.
    boost::asio::write(socket, request);
}
