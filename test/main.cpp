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
    scheduler sched(4);
    set_scheduler(&sched);

    // create channel
    channel_pair<int> pair = make_channel<int>(10);

    int last_written = -1;
    int last_read = -1;

    // writer coroutine
    go(std::string("reading_after_close writer"), [&last_written](channel_writer<int>& writer)
    {
        for(int i = 0; i < 5; i++)
        {
            writer.put(i);
            last_written = i;
        }
    }, std::move(pair.writer));

    // reader coroutine
    go(std::string("reading_after_close reader"), [&last_read](channel_reader<int>& reader)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
    scheduler sched(4);
    set_scheduler(&sched);

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
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        w.put(7);
        writer_finished = true;

    }, std::move(pair.writer));

    sched.wait();
    set_scheduler(nullptr);

    TEST_EQUAL(reader_finished, true);
    TEST_EQUAL(writer_finished, true);
}

// test - writer.put() should exit with exception if reader closes channel
void test_writer_exit_when_closed()
{
    scheduler sched(4);
    set_scheduler(&sched);

    // create channel
    channel_pair<int> pair = make_channel<int>(1);


    go(std::string("test_writer_exit_when_closed reader"), [](channel_reader<int>& r)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

    sched.wait();
    set_scheduler(nullptr);

    TEST_EQUAL(writer_threw, true);
}

// send more items than channels capacity
void test_large_transfer()
{
    std::unique_ptr<scheduler> sched(new scheduler(4));
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
            if ((i % 37) == 0)
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
                if ((i % 53) == 0)
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


void test_nestet_coros()
{
    scheduler sched(4);
    set_scheduler(&sched);

    channel_pair<int> pair1 = make_channel<int>(10);

    go(std::string("test_nestet_coros reader1"), [](channel_reader<int>& reader)
    {
        reader.get();
    }, std::move(pair1.reader));

    go(std::string("test_nestet_coros reader2"), [](channel_writer<int>& writer)
    {
        channel_pair<int> pair2 = make_channel<int>(10);


        go(std::string("test_nestet_coros nested"), [](channel_writer<int>& w1, channel_writer<int>& w2)
        {
            w1.put(1);
            w2.put(3);
        }, std::move(writer), std::move(pair2.writer));

        pair2.reader.get();

    }, std::move(pair1.writer));

    sched.wait();
    set_scheduler(nullptr);
}

void test_muchos_coros()
{
    scheduler sched(4);
    set_scheduler(&sched);

    const int NUM = 1000;
    const int MSGS = 10000;
    std::atomic<int> received(0);
    std::atomic<int> sent(0);
    std::atomic<int> coros(0);
    for(int i = 0; i < NUM; i++)
    {
         channel_pair<int> pair = make_channel<int>(10);

         go(std::string("test_muchos_coros reader"), [&received, &coros](channel_reader<int>& r)
         {
            coros++;
            for(int i = 0; i < MSGS; i++)
            {
                try
                {
                    r.get();
                    received++;
                }
                catch(const channel_closed&)
                {
                    std::cout << "channel closed after readig only " << i << " msgs" << std::endl;
                    throw;
                }
            }
         }, std::move(pair.reader));

         go(std::string("test_muchos_coros writer"), [&sent, &coros](channel_writer<int>& w)
         {
            coros++;
            for(int i = 0; i < MSGS; i++)
            {
                try
                {
                    w.put(i);
                    sent++;
                }
                catch(const channel_closed&)
                {
                    std::cout << "channel closed after writing only " << i << " msgs" << std::endl;
                    throw;
                }
            }
         }, std::move(pair.writer));
    }

    sched.wait();
    set_scheduler(nullptr);

    TEST_EQUAL(coros, NUM*2);
    TEST_EQUAL(received, NUM*MSGS);
    TEST_EQUAL(sent, NUM*MSGS);
}

void test_blocking_coros()
{
    scheduler sched(4);
    set_scheduler(&sched);

    std::atomic<int> nonblocking(0);
    std::atomic<int> blocking(0);

    const int SERIES = 10;
    const int NON_BLOCKING_PER_SER = 10;

    for(int s = 0; s < SERIES; s++)
    {
        // start some non-blocking coroutines
        for(int i = 0; i < NON_BLOCKING_PER_SER; i++)
        {
            go("test_blocking_coros nonblocking", [&nonblocking]()
            {
                nonblocking++;
            });
        }

        // start on that will block
        go("test_blocking_coros blocking", [&blocking]()
        {
            blocking++;
            block();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            unblock();

            blocking++;
            block();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            unblock();
            blocking++;
        });
        // the entire test should block for bit more than 1 second
    }


    sched.wait();
    set_scheduler(nullptr);

    TEST_EQUAL(nonblocking, SERIES*NON_BLOCKING_PER_SER);
    TEST_EQUAL(blocking, SERIES*3);
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

    std::cout << "Staring test: test_nestet_coros" << std::endl;
    test_nestet_coros();

    std::cout << "Staring test: test_muchos_coros" << std::endl;
    test_muchos_coros();

    std::cout << "Staring test: test_blocking_coros" << std::endl;
    test_blocking_coros();

    std::cout << "test completed" << std::endl;
}

