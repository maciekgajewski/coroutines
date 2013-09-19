// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include "libcoroutines/globals.hpp"

#include <iostream>
#include <thread>

using namespace coroutines;

// simple reader-wrtier test, wrtier closes before reader finishes
// expected: reader will read all data, and then throw channel_closed
void test_rw()
{
    std::unique_ptr<threaded_scheduler> sched(new threaded_scheduler);
    set_scheduler(sched.get());

    // create channel
    channel_pair<int> pair = make_channel<int>(10);

    int last_wrtitten = -1;
    int last_read = -1;

    // writer coroutine
    go([&last_wrtitten](channel_writer<int>& writer)
    {
        std::cout << "writing started" << std::endl;
        for(int i = 0; i < 5; i++)
        {
            writer.put(i);
            last_wrtitten = i;
        }
        std::cout << "writing completed" << std::endl;

    }, std::move(pair.writer));

    // reader coroutine
    go([&last_read](channel_reader<int>& reader)
    {
        std::cout << "reader started" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        for(;;)
        {
            try
            {
                last_read = reader.get();
            }
            catch(const channel_closed&)
            {
                break;
            }
        }
        std::cout << "reader finished" << std::endl;
    }, std::move(pair.reader));


    // TEST
    set_scheduler(nullptr);
    sched.reset();

    assert(last_wrtitten == 4);
    assert(last_read == 4);
}

int main(int , char** )
{
    test_rw();
}

