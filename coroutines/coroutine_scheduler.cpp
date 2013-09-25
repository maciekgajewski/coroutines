#include "coroutine_scheduler.hpp"

#include <cassert>

namespace coroutines {

coroutine_scheduler::coroutine_scheduler(unsigned max_running_coroutines)
{
    assert(max_running_coroutines > 0);

    // create contexts
    while(max_running_coroutines--)
    {
        _idle_contexts.emplace_back(this);
    }
}

coroutine_scheduler::~coroutine_scheduler()
{
    wait();
}

void coroutine_scheduler::wait()
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

} // namespace coroutines
