#include "naive_scheduler.hpp"

namespace corountines {

naive_scheduler::naive_scheduler()
{
}

naive_scheduler::~naive_scheduler()
{
    // jooin all threads
    for (std::thread& t : _threads)
    {
        t.join();
    }
}



}
