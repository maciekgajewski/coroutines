// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include "naive_scheduler.hpp"

#include <iostream>
#include <thread>
#include <chrono>

void fun()
{
    std::cout << "boo" << std::endl;
}

void fun2(int x)
{
    std::cout << "x = " << x << std::endl;
}

int main(int , char** )
{
    corountines::naive_scheduler scheduler;

    scheduler.go(fun);
    scheduler.go(fun2, 42);
    std::cout << "Hello" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
}
