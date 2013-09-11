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

void fun2(int x, corountines::naive_channel<int> ch)
{
    std::cout << "x = " << x << std::endl;
    int y = ch.get();
    std::cout << "fun2, received: " << y << std::endl;
}

int main(int , char** )
{
    corountines::naive_scheduler scheduler;

    auto int_channel = scheduler.make_channel<int>(3);

    scheduler.go(fun, int_channel);
    scheduler.go(fun2, 42, int_channel);
    std::cout << "Hello" << std::endl;
    std::cout << "sending ints..." << std::endl;
    int_channel.put(1);
    int_channel.put(2);


}
