// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include "coroutines/globals.hpp"

#define BOOST_TEST_MODULE coroutines_test
#include <boost/test/unit_test.hpp>

#include <signal.h>

static void signal_handler(int)
{
    coroutines::scheduler * sched = coroutines::get_scheduler();
    if (sched)
    {
        sched->debug_dump();
    }
}

// not really a test case, just a hook to indtal sigbnal handler
BOOST_AUTO_TEST_CASE(singal_instller)
{
    signal(SIGINT, signal_handler);
}
