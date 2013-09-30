// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include "coroutines/globals.hpp"
#include "coroutines_io/globals.hpp"
#include "coroutines_io/tcp_socket.hpp"

#include <boost/format.hpp>

#include <iostream>
#include <thread>

using namespace coroutines;

template<typename T1, typename T2>
void _TEST_EQUAL(const T1& a, const T2& b, long line, const char* msg)
{
    if (a != b)
    {
        std::cout << "Line " << line << " : " << msg << " failed (" << a << " != " << b << ")" << std::endl;
        assert(false);
    }
}
#define TEST_EQUAL(a, b) _TEST_EQUAL(a, b,  __LINE__, #a "==" #b)
#define RUN_TEST(test_name) std::cout << ">>> Starting test: " << #test_name << std::endl; test_name();

void test_connect()
{
    scheduler sched(4);
    service srv(sched);
    set_scheduler(&sched);
    set_service(&srv);

    srv.start();


    go("test_connect", []()
    {
        try
        {
            tcp_socket s;

            std::cout << "connecting..." << std::endl;
            s.connect(boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address_v4::from_string("127.0.0.1"),
                22));
            std::cout << "connected" << std::endl;
        }
        catch(const std::exception& e)
        {
            std::cout << "connection error: " << e.what() << std::endl;
        }
    });


    srv.stop();
    sched.wait();
    set_service(nullptr);
    set_scheduler(nullptr);
}

int main(int , char** )
{
    RUN_TEST(test_connect);

    std::cout << "test completed" << std::endl;
}

