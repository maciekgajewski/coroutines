// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
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

void coroutine_scheduler::schedule(coroutine&& coro)
{
    std::cout << "SCHED: scheduling corountine '" << coro.name() << "'" << std::endl;

    // put the coro on indle context, if available
    {
        std::lock_guard<std::mutex> contexts_lock(_contexts_mutex);
        if (!_idle_contexts.empty())
        {
            std::cout << "SCHED: scheduling corountine, idle context exists, adding there" << std::endl;
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

            return;
        }
    }

    // called from withing working context
    if (context::current_context())
    {
        context::current_context()->enqueue(std::move(coro));
    }
    else
    {
        std::cout << "SCHED: scheduling corountine: adding to global list" << std::endl;
        // put into global queue
        _global_queue.push(std::move(coro));
    }
}


} // namespace coroutines
