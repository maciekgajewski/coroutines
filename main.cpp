// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include "naive_scheduler.hpp"

#include <iostream>
#include <thread>
#include <chrono>

void fun(corountines::naive_channel<int> ch)
{
    std::cout << "boo" << std::endl;

    int x = ch.get();
    std::cout << "received: " << x << std::endl;
}

void fun2(int x)
{
    std::cout << "x = " << x << std::endl;
}

int main(int , char** )
{
    corountines::naive_scheduler scheduler;

    auto int_channel = scheduler.make_channel<int>(3);

    scheduler.go(fun, int_channel);
    scheduler.go(fun2, 42);
    std::cout << "Hello" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "sending int..." << std::endl;
    int_channel.put(5);
    std::this_thread::sleep_for(std::chrono::seconds(1));


}
