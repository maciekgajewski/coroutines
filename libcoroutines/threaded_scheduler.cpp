// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "threaded_scheduler.hpp"

namespace coroutines {

threaded_scheduler::threaded_scheduler()
{
}

threaded_scheduler::~threaded_scheduler()
{
    // jooin all threads
    for (std::thread& t : _threads)
    {
        t.join();
    }
}


}
