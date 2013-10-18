// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include "coroutines/globals.hpp"

#include "coroutines_io/globals.hpp"
#include "coroutines_io/io_scheduler.hpp"
#include "coroutines_io/tcp_socket.hpp"
#include "coroutines_io/tcp_acceptor.hpp"


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
    io_scheduler srv(sched);
    set_scheduler(&sched);
    set_io_scheduler(&srv);
    srv.start();

    auto pair = make_channel<int>(1);

    go("test_connect acceptor", [&pair]()
    {
        try
        {
            tcp_acceptor acceptor;

            acceptor.listen(boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address_v4::from_string("0.0.0.0"),
                22445));

            pair.writer.put(0);

            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "accepting..." << std::endl;
            tcp_socket s = acceptor.accept();
            std::cout << "accepted" << std::endl;

            static const int BUFSIZE = 64;
            char buf[BUFSIZE];

            std::cout << "receiving...." << std::endl;
            std::size_t received = s.read(buf, BUFSIZE);

            std::string rstr(buf, received);
            std::cout << "received: " << rstr << std::endl;
        }
        catch(const std::exception& e)
        {
            std::cout << "acceptor error: " << e.what() << std::endl;
        }

    });

    go("test_connect connector", [&pair]()
    {
        try
        {
            tcp_socket s;

            // wait for acceptor
            pair.reader.get();

            std::cout << "connecting..." << std::endl;
            s.connect(boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address_v4::from_string("127.0.0.1"),
                22445));
            std::cout << "connected" << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(1));

            std::cout << "sending hello..." << std::endl;

            static const std::string hello = "hello";
            std::size_t sent = s.write(hello.c_str(), hello.length());

            std::cout << "sent " << sent << " bytes" << std::endl;
        }
        catch(const std::exception& e)
        {
            std::cout << "connection error: " << e.what() << std::endl;
        }
    });

    sched.wait();
    srv.stop();
    sched.wait();
    set_io_scheduler(nullptr);
    set_scheduler(nullptr);
}

int main(int , char** )
{
    RUN_TEST(test_connect);

    std::cout << "test completed" << std::endl;
}

