// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include "coroutines/globals.hpp"
#include "coroutines/categorized_container.hpp"
#include <boost/format.hpp>

#include <iostream>
#include <thread>

#include <signal.h>

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

void nonblocking_coro(std::atomic<int>& counter, int spawns)
{
    if (spawns > 0)
    {
        counter++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // they shoudl saturate all 4 threads for at least 500ms, so we need 2000 ms total
        go("nonblocking nested", nonblocking_coro, std::ref(counter), spawns-1);
    }
}

void test_blocking_coros()
{
    scheduler sched(4);
    set_scheduler(&sched);

    std::cout << "(this test should take approx. one second)" << std::endl;

    std::atomic<int> nonblocking(0);
    std::atomic<int> blocking(0);

    const int SERIES = 10;
    const int NON_BLOCKING_PER_SER = 10;
    const int NON_BLOCKING_SPAWNS = 10;

    for(int s = 0; s < SERIES; s++)
    {
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

        // start some non-blocking coroutines
        for(int i = 0; i < NON_BLOCKING_PER_SER; i++)
        {
            go("test_blocking_coros nonblocking", [&nonblocking]()
            {
                nonblocking_coro(nonblocking, NON_BLOCKING_SPAWNS);
            });
        }

        // the entire test should block for bit more than 1 second
    }


    sched.wait();
    set_scheduler(nullptr);

    TEST_EQUAL(nonblocking, SERIES*NON_BLOCKING_PER_SER*NON_BLOCKING_SPAWNS);
    TEST_EQUAL(blocking, SERIES*3);
}

void test_multiple_readers()
{
    scheduler sched(4);
    set_scheduler(&sched);

    static const int MSGS = 10000;
    static const int READERS = 100;
    std::atomic<int> received(0);
    std::atomic<int> closed(0);

    channel_pair<int> pair = make_channel<int>(10);

    go("test_multiple_readers writer", [](channel_writer<int>& writer)
    {
        for(int i = 0; i < MSGS; i++)
        {
            writer.put(i);
        }
    }, std::move(pair.writer));

    for(int i = 0; i < READERS; i++)
    {
        go("test_multiple_readers reader", [&received, &closed](channel_reader<int> reader)
        {
            try
            {
                for(;;)
                {
                    reader.get();
                    received++;
                }
            }
            catch(const channel_closed&)
            {
                closed++;
                throw;
            }

        }, pair.reader);
    }

    sched.wait();
    set_scheduler(nullptr);

    TEST_EQUAL(closed, READERS);
    TEST_EQUAL(received, MSGS);
}

void test_multiple_writers()
{
    scheduler sched(4);
    set_scheduler(&sched);

    static const int MSGS_PER_WRITER = 100;
    static const int WRITERS = 100;

    std::atomic<int> received(0);

    channel_pair<int> pair = make_channel<int>(10);

    for(int i = 0; i < WRITERS; i++)
    {
        go(
            boost::str(boost::format("test_multiple_readers writer %d") % i),
            [](channel_writer<int> writer)
        {
            for(int i = 0; i < MSGS_PER_WRITER; i++)
            {
//                std::cout <<
//                    (boost::format("%s writes %d...") % coroutine::current_corutine()->name() % i)  << std::endl;
                writer.put(i);
            }
//            std::cout << coroutine::current_corutine()->name() << " finished" << std::endl;
        }, pair.writer);
    }
    pair.writer.close();

    go("test_multiple_readers reader", [&received](channel_reader<int>& reader)
    {
        for(;;)
        {
//            std::cout << "reader reads..." << std::endl;
            reader.get();
            received++;
        }

    }, std::move(pair.reader));

    sched.wait();
    set_scheduler(nullptr);

    TEST_EQUAL(received, WRITERS*MSGS_PER_WRITER);
}


/////////////

struct node
{
    double value;
    node* left = nullptr;
    node* right = nullptr;
};

node* build_tree(unsigned levels)
{
    static double x = 7;

    if (levels == 0 )
        return nullptr;

    node* n = new node();
    n->value = levels * x;
    n->left = build_tree(levels - 1);
    n->right = build_tree(levels - 1);

    x = x*3.14;

    return n;
}

double sum_tree(node* n)
{
    if (n)
    {
        return n->value + sum_tree(n->left) + sum_tree(n->right);
    }
    else
    {
        return 0.0;
    }
}

void paraller_sum_sub(node* tree, channel_writer<double>& out)
{
    out.put(sum_tree(tree));
}

double paraller_sum(node* tree)
{
    if (tree)
    {
        auto pair = make_channel<double>(2);
        go(paraller_sum_sub, tree->left, pair.writer);
        go(paraller_sum_sub, tree->right, pair.writer);

        return pair.reader.get() + pair.reader.get();
    }
    else
    {
        return 0;
    }

}

void tree_traverse_test()
{
    scheduler sched(4);
    set_scheduler(&sched);


    node* tree = build_tree(20);

    auto start = std::chrono::high_resolution_clock::now();
    double sum = sum_tree(tree);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::duration single = end - start;
    std::chrono::high_resolution_clock::duration parallel;
    
    double psum = 0.0;
    go([tree, &parallel, &psum]()
    {
        auto start = std::chrono::high_resolution_clock::now();
        psum = paraller_sum(tree);
        auto end = std::chrono::high_resolution_clock::now();
        parallel = end-start;
    });


    sched.wait();
    set_scheduler(nullptr);

    TEST_EQUAL(sum, psum);

    std::cout << "single thread duration: " << single / std::chrono::milliseconds(1) << " ms " << std::endl;
    std::cout << "parallel duration: " << parallel / std::chrono::milliseconds(1) << " ms " << std::endl;

    TEST_EQUAL(sum, psum);
}

void test_non_blocking_read()
{
    scheduler sched(4);
    set_scheduler(&sched);

    auto pair = make_channel<double>(10);
    int read = 0;
    bool completed = false;

    go([&]()
    {
        pair.writer.put(1.1);
        pair.writer.put(2.2);
        pair.writer.put(3.3);

        double x;
        while(pair.reader.try_get(x))
            read++;
        completed = true;
    });

    sched.wait();
    set_scheduler(nullptr);

    TEST_EQUAL(read, 3);
    TEST_EQUAL(completed, true);
}


class int_wrapper
{
public:
    int_wrapper(int v) : _v(v) {}
    int_wrapper(const int_wrapper&) = delete;

    int get() const { return _v; }
    void set(int v) { _v = v; }

private:

    int _v;
};


#define RUN_TEST(test_name) std::cout << ">>> Starting test: " << #test_name << std::endl; test_name();

void signal_handler(int)
{
    scheduler * sched = get_scheduler();
    if (sched)
    {
        sched->debug_dump();
    }
}

int main(int , char** )
{
    // install signal handler, for debugging
    signal(SIGINT, signal_handler);

//    RUN_TEST(test_reading_after_close);
//    RUN_TEST(test_reader_blocking);
//    RUN_TEST(test_writer_exit_when_closed);
//    RUN_TEST(test_large_transfer);
//    RUN_TEST(test_nestet_coros);
//    RUN_TEST(test_muchos_coros);
    RUN_TEST(test_blocking_coros);
//    RUN_TEST(test_multiple_readers);
//    RUN_TEST(test_multiple_writers);
//    RUN_TEST(tree_traverse_test);
//    RUN_TEST(test_non_blocking_read);

    std::cout << "test completed" << std::endl;
}

