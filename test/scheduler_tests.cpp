// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "coroutines/globals.hpp"

#include "test/fixtures.hpp"

#include <boost/test/unit_test.hpp>

namespace coroutines { namespace tests {


BOOST_FIXTURE_TEST_CASE(test_nestet_coros, fixture)
{
    channel_pair<int> pair1 = make_channel<int>(10);

    go(std::string("test_nestet_coros reader1"), [](channel_reader<int>& reader)
    {
        int res = reader.get();
        BOOST_CHECK_EQUAL(res, 1);
    }, std::move(pair1.reader));

    go(std::string("test_nestet_coros reader2"), [](channel_writer<int>& writer)
    {
        channel_pair<int> pair2 = make_channel<int>(10);


        go(std::string("test_nestet_coros nested"), [](channel_writer<int>& w1, channel_writer<int>& w2)
        {
            w1.put(1);
            w2.put(3);
        }, std::move(writer), std::move(pair2.writer));

        int res = pair2.reader.get();
        BOOST_CHECK_EQUAL(res, 3);

    }, std::move(pair1.writer));
}

BOOST_FIXTURE_TEST_CASE(test_muchos_coros, fixture)
{
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

    wait_for_completion();

    BOOST_CHECK_EQUAL(coros, NUM*2);
    BOOST_CHECK_EQUAL(received, NUM*MSGS);
    BOOST_CHECK_EQUAL(sent, NUM*MSGS);
}

static void nonblocking_coro(std::atomic<int>& counter, int spawns)
{
    if (spawns > 0)
    {
        counter++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // they should saturate all 4 threads for at least 500ms, so we need 2000 ms total
        go("nonblocking nested", nonblocking_coro, std::ref(counter), spawns-1);
    }
}

BOOST_FIXTURE_TEST_CASE(test_blocking_coros, fixture)
{
    std::cout << " > this test should take approx. one second" << std::endl;

    std::atomic<int> nonblocking(0);
    std::atomic<int> blocking(0);

    const int SERIES = 10;
    const int NON_BLOCKING_PER_SER = 10;
    const int NON_BLOCKING_SPAWNS = 10;

    auto start = std::chrono::high_resolution_clock::now();

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

    wait_for_completion();

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << " > actual time: " << (end-start)/std::chrono::milliseconds(1) << " ms" << std::endl;

    BOOST_CHECK_EQUAL(nonblocking, SERIES*NON_BLOCKING_PER_SER*NON_BLOCKING_SPAWNS);
    BOOST_CHECK_EQUAL(blocking, SERIES*3);
}

/////////////
// tree traversal

struct node
{
    double value;
    node* left = nullptr;
    node* right = nullptr;
};

static node* build_tree(unsigned levels)
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

static double sum_tree(node* n)
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

static void paraller_sum_sub(node* tree, channel_writer<double>& out)
{
    out.put(sum_tree(tree));
}

static double paraller_sum(node* tree)
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

BOOST_FIXTURE_TEST_CASE(tree_traverse_test, fixture)
{
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

    wait_for_completion();

    BOOST_CHECK_EQUAL(sum, psum);

    std::cout << "single thread duration: " << single / std::chrono::milliseconds(1) << " ms " << std::endl;
    std::cout << "parallel duration: " << parallel / std::chrono::milliseconds(1) << " ms " << std::endl;

    BOOST_CHECK_EQUAL(sum, psum);
}


}}

