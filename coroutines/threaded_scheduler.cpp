// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "threaded_scheduler.hpp"

namespace coroutines {

threaded_scheduler::threaded_scheduler()
{
}

threaded_scheduler::~threaded_scheduler()
{
    wait();
}

void threaded_scheduler::wait()
{
    // join all threads
    while(!_threads.empty())
    {
        _threads.front().join();
        {
            std::lock_guard<std::mutex> lock(_threads_mutex);
            _threads.pop_front();
        }
    }
}


}
