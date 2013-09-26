// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include "coroutines/globals.hpp"

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

// simple reader-wrtier test, wrtier closes before reader finishes
// expected: reader will read all data, and then throw channel_closed
void test_reading_after_close()
{
    scheduler sched;
    set_scheduler(&sched);

    // create channel
    channel_pair<int> pair = make_channel<int>(10);

    int last_written = -1;
    int last_read = -1;

    // writer coroutine
    go(std::string("reading after close writer"), [&last_written](channel_writer<int>& writer)
    {
        for(int i = 0; i < 5; i++)
        {
            writer.put(i);
            last_written = i;
        }
    }, std::move(pair.writer));

    // reader coroutine
    go(std::string("reading after close reader"), [&last_read](channel_reader<int>& reader)
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
    sched.wait();

    TEST_EQUAL(last_written, 4);
    TEST_EQUAL(last_read, 4);
}

// Reader blocking:  reader should block until wrtier writes
void test_reader_blocking()
{
    std::unique_ptr<scheduler> sched(new scheduler);
    set_scheduler(sched.get());

    // create channel
    channel_pair<int> pair = make_channel<int>(10);

    bool reader_finished = false;
    go(std::string("test_reader_blocking reader"), [&reader_finished](channel_reader<int>& r)
    {
        r.get();
        reader_finished = true;
    }, std::move(pair.reader));

    TEST_EQUAL(reader_finished, false);

    bool writer_finished = false;
    go(std::string("test_reader_blocking writer"), [&writer_finished](channel_writer<int>& w)
    {
        w.put(7);
        writer_finished = true;

    }, std::move(pair.writer));

    set_scheduler(nullptr);
    sched.reset();

    TEST_EQUAL(reader_finished, true);
    TEST_EQUAL(writer_finished, true);
}

// test - writer.put() should exit with exception if reader closes channel
void test_writer_exit_when_closed()
{
    std::unique_ptr<scheduler> sched(new scheduler);
    set_scheduler(sched.get());

    // create channel
    channel_pair<int> pair = make_channel<int>(1);


    go(std::string("test_writer_exit_when_closed reader"), [](channel_reader<int>& r)
    {
        // do nothing, close the chanel on exit
    }, std::move(pair.reader));

    bool writer_threw = false;
    go(std::string("test_writer_exit_when_closed writer"), [&writer_threw](channel_writer<int>& w)
    {
        try
        {
            w.put(1);
            w.put(2); // this will block
        }
        catch(const channel_closed&)
        {
            writer_threw = true;
        }

    }, std::move(pair.writer));

    set_scheduler(nullptr);
    sched.reset();

    TEST_EQUAL(writer_threw, true);
}

// send more items than channels capacity
void test_large_transfer()
{
    std::unique_ptr<scheduler> sched(new scheduler);
    set_scheduler(sched.get());

    // create channel
    channel_pair<int> pair = make_channel<int>(10);

    int last_written = -1;
    int last_read = -1;
    static const int MESSAGES = 10000;

    // writer coroutine
    go(std::string("large_transfer writer"), [&last_written](channel_writer<int>& writer)
    {
        for(int i = 0; i < MESSAGES; i++)
        {
            writer.put(i);
            last_written = i;
            if ((i % 7) == 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                std::cout << "long write progress: " << i << "/" << 10000 << std::endl;
            }
        }
    }, std::move(pair.writer));

    // reader coroutine
    go(std::string("large_transfer reader"), [&last_read](channel_reader<int>& reader)
    {
        for(int i = 0;;i++)
        {
            try
            {
                last_read = reader.get();
                if ((i % 13) == 0)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    std::cout << "long read progress: " << i << "/" << 10000 << std::endl;
                }
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

    TEST_EQUAL(last_written, MESSAGES-1);
    TEST_EQUAL(last_read, MESSAGES-1);
}

int main(int , char** )
{
    std::cout << "Staring test: test_reading_after_close" << std::endl;
    test_reading_after_close();

    std::cout << "Staring test: test_reader_blocking" << std::endl;
    test_reader_blocking();

    std::cout << "Staring test: test_writer_exit_when_closed" << std::endl;
    test_writer_exit_when_closed();

    std::cout << "Staring test: test_large_transfer" << std::endl;
    test_large_transfer();

    std::cout << "test completed" << std::endl;
}

