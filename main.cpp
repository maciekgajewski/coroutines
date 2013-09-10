// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include "naive_scheduler.hpp"

#include <iostream>

void fun()
{
    std::cout << "boo";
}

int main(int , char** )
{
    corountines::naive_scheduler scheduler;

    //scheduler.go(fun);
    std::thread t([]()
    {
        std::cout << "from thread" << std::endl;
    });
    std::cout << "Hello" << std::endl;

    t.join();
}
