// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include "libcoroutines/channel.hpp"
#include "libcoroutines/threaded_channel.hpp"
#include "libcoroutines/threaded_scheduler.hpp"

#include <iostream>
#include <thread>

using namespace coroutines;


int main(int , char** )
{
    // create scheduler
    threaded_scheduler scheduler;

    // create channel
    channel_pair<int> pair = scheduler.make_channel<int>(10);

    // writer coroutine
    scheduler.go([](channel_writer<int>& writer)
    {
        std::cout << "writing started" << std::endl;
        for(int i = 0; i < 100; i++)
        {
            writer.put(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        std::cout << "writing completed" << std::endl;

    }, std::move(pair.writer));

    // reader coroutine
    scheduler.go(
    [](channel_reader<int>& reader)
    {
        std::cout << "reader started" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for(;;)
        {
            try
            {
                std::cout << reader.get() << std::endl;
            }
            catch(const channel_closed&)
            {
                break;
            }
        }
        std::cout << "reader finished" << std::endl;
    }, std::move(pair.reader));


}

