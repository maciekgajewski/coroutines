#include "coroutine_scheduler.hpp"

#include <cassert>

namespace coroutines {

extern thread_local context* __current_context;

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

void coroutine_scheduler::schedule(coroutine&& coro)
{
    // put the coro on indle context, if available
    {
        std::lock_guard<std::mutex> contexts_lock(_contexts_mutex);
        if (!_idle_contexts.empty())
        {
            std::lock_guard<std::mutex> threads_lock(_threads_mutex);

            // from idle, push to active
            context ctx = std::move(_idle_contexts.front());
            _idle_contexts.pop_front();

            auto it = _active_contexts.insert(_active_contexts.end(), std::move(ctx));

            it->enqueue(std::move(coro));

            _threads.emplace_back([it, this]()
            {
                it->run();
                // move back to idle when finished
                {
                    std::lock_guard<std::mutex> contexts_lock(_contexts_mutex);
                    _active_contexts.splice(_idle_contexts.end(), _active_contexts, it);
                }
            });
        }
        return;
    }

    // called from withing working context
    if (__current_context)
    {
        __current_context->enqueue(std::move(coro));
    }
    else
    {
        // put into global queue
        std::lock_guard<std::mutex> gq_lock(_global_queue_mutex);
        _global_queue.push_back(std::move(coro));
    }
}


} // namespace coroutines
