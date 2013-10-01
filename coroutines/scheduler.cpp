// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "scheduler.hpp"
#include "algorithm.hpp"

#include <cassert>
#include <iostream>
#include <cstdlib>

namespace coroutines {

scheduler::scheduler(unsigned max_running_coroutines)
    : _max_running_coroutines(max_running_coroutines)
{
    assert(max_running_coroutines > 0);
}

scheduler::~scheduler()
{
    wait();
}

void scheduler::wait()
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

void scheduler::steal(std::list<coroutine_ptr>& out)
{
    std::lock_guard<std::mutex> lock(_contexts_mutex);

    unsigned all_active = _active_contexts.size();

    int idx = std::rand() % all_active;
    auto it = _active_contexts.begin();
    std::advance(it, idx);
    for(int i = 0; i < all_active; ++i)
    {
        unsigned stolen = (*it)->steal(out);
        if (stolen > 0 )
            break;
        it ++;
        if (it == _active_contexts.end())
            it = _active_contexts.begin();
    }
}

void scheduler::context_finished(context* ctx)
{
    //std::cout << "SCHED: context " << ctx << " finished" << std::endl;

    std::lock_guard<std::mutex> lock(_contexts_mutex);
    auto it = find_ptr(_active_contexts, ctx);
    if (it == _active_contexts.end())
    {
        //std::cout << "SCHED: it was a blocked context" << std::endl;
        it = find_ptr(_blocked_contexts, ctx);
        assert(it != _blocked_contexts.end());
        _blocked_contexts.erase(it);
    }
    else
    {
        //std::cout << "SCHED: it was an active context" << std::endl;
        _active_contexts.erase(it);
    }
}

void scheduler::context_blocked(context* ctx, std::list<coroutine_ptr>& coros)
{
    // move context to blocked
    {
        std::lock_guard<std::mutex> lock(_contexts_mutex);
        auto it = find_ptr(_active_contexts, ctx);
        assert(it != _active_contexts.end());

        context_ptr moved = std::move(*it);
        _active_contexts.erase(it);
        _blocked_contexts.push_back(std::move(moved));
        //std::cout << "SCHED: context " << ctx << " blocked. There is " << _blocked_contexts.size() << " blocked and " << _active_contexts.size() << " active contexts now" << std::endl;
    }

    // do something with the remaining coros
    schedule(coros);
}

void scheduler::schedule(std::list<coroutine_ptr>& coros)
{
    for(coroutine_ptr& coro : coros)
    {
        schedule(std::move(coro));
    }
}


bool scheduler::context_unblocked(context* ctx)
{
    {
        std::lock_guard<std::mutex> lock(_contexts_mutex);
        auto it = find_ptr(_blocked_contexts, ctx);
        assert(it != _blocked_contexts.end());

        //std::cout << "SCHED: context " << ctx << " unblocked. There is " << _blocked_contexts.size() << " blocked and " <<_active_contexts.size() << " active contexts now" << std::endl;

        if (_active_contexts.size() < _max_running_coroutines)
        {
            context_ptr moved = std::move(*it);
            _blocked_contexts.erase(it);

            //std::cout << "SCHED: context " << ctx << " allowed to continue" << std::endl;
            _active_contexts.push_back(std::move(moved));
            // the context and current coroutine may continue unharassed
            return true;
        }
    }

    // no mutex should be locked below this line, as we may preempt

    //std::cout << "SCHED: context " << ctx << " NOT allowed to continue, coroutine will yield now" << std::endl;
    coroutine* coro = coroutine::current_corutine();
    assert(coro);

    coro->yield([this](std::unique_ptr<coroutine>& this_coro)
    {
        _global_queue.push(std::move(this_coro));
        // coroutine will continue on another thread, context will be destroyed
    });

    return false;
}

void scheduler::schedule(coroutine_ptr&& coro)
{
    //std::cout << "SCHED: scheduling corountine '" << coro->name() << "'" << std::endl;

    // create new active context, if limit not reached yet
    {
        std::lock_guard<std::mutex> contexts_lock(_contexts_mutex);
        if (_active_contexts.size() < _max_running_coroutines)
        {
            //std::cout << "SCHED: scheduling corountine, idle context exists, adding there" << std::endl;
            std::lock_guard<std::mutex> threads_lock(_threads_mutex);

            context_ptr ctx(new context(this));
            context* ctx_ptr = ctx.get();
            ctx->enqueue(std::move(coro));
            _active_contexts.push_back(std::move(ctx));

            _threads.emplace_back([ctx_ptr]()
            {
                ctx_ptr->run();
            });

            return;
        }
    }

    // called from withing working context
    if (context::current_context())
    {
        //std::cout << "SCHED: scheduling corountine: adding to current context's list" << std::endl;
        context::current_context()->enqueue(std::move(coro));
    }
    else
    {
        //std::cout << "SCHED: scheduling corountine: adding to global list" << std::endl;
        // put into global queue
        _global_queue.push(std::move(coro));
    }
}


} // namespace coroutines
