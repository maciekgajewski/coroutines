// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include "libcoroutines/globals.hpp"

#include <iostream>
#include <thread>

using namespace coroutines;

template<typename T1, typename T2>
void _TEST_EQUAL(const T1& a, const T2& b, const char* msg)
{
    if (a != b)
    {
        std::cout << msg << " failed" << std::endl;
        assert(false);
    }
}
#define TEST_EQUAL(a, b) _TEST_EQUAL(a, b, #a "==" #b)

// simple reader-wrtier test, wrtier closes before reader finishes
// expected: reader will read all data, and then throw channel_closed
void test_reading_after_close()
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
        for(int i = 0; i < 5; i++)
        {
            writer.put(i);
            last_wrtitten = i;
        }
    }, std::move(pair.writer));

    // reader coroutine
    go([&last_read](channel_reader<int>& reader)
    {
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
    }, std::move(pair.reader));


    // TEST
    set_scheduler(nullptr);
    sched.reset();

    TEST_EQUAL(last_wrtitten, 4);
    TEST_EQUAL(last_read, 4);
}

// Reader blocking:  reader should block until wrtier writes
void test_reader_blocking()
{
    std::unique_ptr<threaded_scheduler> sched(new threaded_scheduler);
    set_scheduler(sched.get());

    // create channel
    channel_pair<int> pair = make_channel<int>(10);

    bool reader_finished = false;
    go([&reader_finished](channel_reader<int>& r)
    {
        r.get();
        reader_finished = true;
    }, std::move(pair.reader));

    TEST_EQUAL(reader_finished, false);

    bool writer_finished = false;
    go([&writer_finished](channel_writer<int>& w)
    {
        w.put(7);
        writer_finished = true;

    }, std::move(pair.writer));

    set_scheduler(nullptr);
    sched.reset();

    TEST_EQUAL(reader_finished, true);
    TEST_EQUAL(writer_finished, true);
}

int main(int , char** )
{
    test_reading_after_close();
    test_reader_blocking();
}

