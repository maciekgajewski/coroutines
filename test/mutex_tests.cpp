// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "coroutines/mutex.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include <thread>
#include <random>
#include <numeric>
#include <vector>
#include <mutex>
#include <iostream>
#include <chrono>

namespace coroutines { namespace tests {

// torture test

template<typename ContainerType>
void writer(ContainerType& data, rw_spinlock& mutex)
{
    std::lock_guard<rw_spinlock> lock(mutex);

    static std::minstd_rand generator;
    static std::uniform_int_distribution<int> distribution(-100, 100);

    int sum = 0;
    for(int i = 0; i < data.size() - 1; i++)
    {
        data[i] = distribution(generator);
        sum += data[i];
    }
    data.back() = -sum;
}

template<typename ContainerType>
void reader(ContainerType& data, rw_spinlock& mutex)
{
    reader_guard<rw_spinlock> lock(mutex);

    int sum = std::accumulate(data.begin(), data.end(), 0);
    BOOST_REQUIRE_EQUAL(sum, 0);
}

BOOST_AUTO_TEST_CASE(rw_spinlock_torture)
{
    static const int data_size = 100000;
    static const int thread_num = 10;
    static const int cycles_per_thread = 100;
    static const int writers_per_cycle = 1;
    static const int readers_per_cycle = 10;

    std::vector<int> data(data_size, 0);

    rw_spinlock mutex;

    // init the data
    writer(data, mutex);

    std::cout << "rw_spinlock torture-test, wait..." << std::endl;

    std::atomic<int> writers_run(0);
    std::atomic<int> readers_run(0);

    std::vector<std::thread> threads;
    for(int i = 0; i < thread_num; i++)
    {
        threads.emplace_back([&]()
        {
            for(int c = 0; c < cycles_per_thread; c++)
            {
                for(int r = 0; r < readers_per_cycle; r++)
                {
                    reader(data, mutex);
                    readers_run++;
                }
                for(int w = 0; w < writers_per_cycle; w++)
                {
                    writer(data, mutex);
                    writers_run++;
                }
            }
        });
    }

    for(std::thread& t : threads)
    {
        t.join();
    }

    BOOST_CHECK_EQUAL(writers_run, thread_num * cycles_per_thread * writers_per_cycle);
    BOOST_CHECK_EQUAL(readers_run, thread_num * cycles_per_thread * readers_per_cycle);

    std::cout << "rw_spinlock torture-test finished" << std::endl;
}

// this test checks whether the rw spinlock is actually a reader-writer
BOOST_AUTO_TEST_CASE(rw_spinlock_test)
{
    static const int concurrent_writers = 4;
    static const int concurrent_readers = 10;
    static const auto delay = std::chrono::milliseconds(100);

    rw_spinlock mutex;
    std::vector<std::thread> threads;

    std::cout << "rw_spinlock test..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    // start the readers first - the total time should be ~= delay
    for(int i = 0; i < concurrent_readers; i++)
    {
        threads.emplace_back([&]()
        {
            reader_guard<rw_spinlock> lock(mutex);
            std::this_thread::sleep_for(delay);
        });
    }

    // expected total time: delay*concurrent_writers
    for(int i = 0; i < concurrent_writers; i++)
    {
        threads.emplace_back([&]()
        {
            std::lock_guard<rw_spinlock> lock(mutex);
            std::this_thread::sleep_for(delay);
        });
    }

    for(std::thread& thread : threads)
    {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();

    int total_ms = (end-start) / std::chrono::milliseconds(1);
    int expected_ms = (concurrent_writers+1)*delay / std::chrono::milliseconds(1);
    int tolerance = delay / std::chrono::milliseconds(1);

    std::cout << " > total time: " << total_ms << " ms" << std::endl;
    std::cout << " > expected time: " << expected_ms << " ms" << std::endl;

    BOOST_CHECK_CLOSE(float(total_ms), float(expected_ms), 100*float(tolerance)/expected_ms);
}


}}



