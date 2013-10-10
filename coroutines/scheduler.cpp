// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "coroutines/scheduler.hpp"
#include "coroutines/algorithm.hpp"

#define CORO_LOGGING
#include "coroutines/logging.hpp"

#include <cassert>
#include <iostream>
#include <cstdlib>

namespace coroutines {

scheduler::scheduler(unsigned max_running_coroutines)
    : _max_allowed_running_coroutines(max_running_coroutines)
{
    assert(max_running_coroutines > 0);

    // setup
    {
        std::lock_guard<mutex> lock(_processors_mutex);
        for(unsigned i = 0; i < max_running_coroutines; i++)
        {
            _processors.emplace_back(this);
        }
    }
}

scheduler::~scheduler()
{
    wait();
}

void scheduler::debug_dump()
{
    std::lock_guard<mutex> lock(_coroutines_mutex);
    std::cerr << "=========== scheduler debug dump ============" << std::endl;
    std::cerr << "          active coroutines now: " << _coroutines.size() << std::endl;
    std::cerr << "     max active coroutines seen: " << _max_active_coroutines << std::endl;
    std::cerr << "  max allowed coros in parallel: " << _max_allowed_running_coroutines << std::endl;

    std::cerr << std::endl;
    std::cerr << " Active coroutines:" << std::endl;
    for(auto& coro : _coroutines)
    {
        std::cerr << " * " << coro->name() << " : " << coro->last_checkpoint() << std::endl;
    }
    std::cerr << "=============================================" << std::endl;
    std::terminate();
}

void scheduler::wait()
{
    std::unique_lock<mutex> lock(_coroutines_mutex);
    _coro_cv.wait(lock, [this]() { return _coroutines.empty(); });
}

void scheduler::coroutine_finished(coroutine* coro)
{
//    std::cout << "CORO=" << coro << " will be destroyed now" << std::endl;

    std::lock_guard<mutex> lock(_coroutines_mutex);
    auto it = find_ptr(_coroutines, coro);
    assert(it != _coroutines.end());
    _coroutines.erase(it);

    if (_coroutines.empty())
    {
        _coro_cv.notify_all();
    }
}

void scheduler::processor_idle(processor_weak_ptr pc, bool blocked)
{
    CORO_LOG("SCHED: processor ", pc, " finished");

    {
        std::lock_guard<mutex> lock(_processors_mutex);

        if (blocked)
        {
            assert(_active_blocked_processors > 0);
            auto it = find_ptr(_blocked_processors, pc);
            assert(it != _blocked_processors.end());

            std::swap(*it, _blocked_processors[_active_blocked_processors]);
            _active_blocked_processors--;


            // TODO garbage-collect if there is too many idle blocked
        }
        else
        {
            assert(_active_processors > 0);
            auto it = find_ptr(_active_processors, pc);
            assert(it != _active_processors.end());

            std::swap(*it, _active_processors[_active_processors]);
            _active_processors--;
        }

    }

}

void scheduler::processor_blocked(processor_weak_ptr pr, std::vector<coroutine_weak_ptr>& queue)
{

}

void scheduler::context_blocked(context* ctx, std::list<coroutine_weak_ptr>& coros, const std::string& /*checkpoint_name*/)
{
    // move context to blocked
    {
        std::lock_guard<mutex> lock(_contexts_mutex);
        auto it = find_ptr(_active_contexts, ctx);
        assert(it != _active_contexts.end());

        context_ptr moved = std::move(*it);
        _active_contexts.erase(it);
        _blocked_contexts.push_back(std::move(moved));
        CORO_LOG("SCHED: context ", ctx, " blocked. There is ",  _blocked_contexts.size(), " blocked and ", _active_contexts.size(), " active contexts now");
    }

    // do something with the remaining coros
    schedule(coros);
}

bool scheduler::context_unblocked(context* ctx, const std::string& checkpoint_name)
{
    {
        std::lock_guard<mutex> lock(_contexts_mutex);

        auto it = find_ptr(_blocked_contexts, ctx);
        assert(it != _blocked_contexts.end());

        CORO_LOG("SCHED: context ", ctx, " unblocked. There is ", _blocked_contexts.size(), " blocked and ", _active_contexts.size(), " active contexts now");

        if (_active_contexts.size() < _max_allowed_running_coroutines)
        {
            // ove to active
            context_ptr moved = std::move(*it);
            _blocked_contexts.erase(it);
            _active_contexts.push_back(std::move(moved));

            CORO_LOG("SCHED: context ", ctx, " allowed to continue");
            // the context and current coroutine may continue unharassed
            return true;
        }
    }

    // no mutex should be locked below this line, as we may preempt

    CORO_LOG("SCHED: context ", ctx, " NOT allowed to continue, coroutine will yield now");
    coroutine* coro = coroutine::current_corutine();
    assert(coro);

    coro->yield(checkpoint_name, [this](coroutine_weak_ptr coro)
    {
        _global_queue.push(std::move(coro));
        // coroutine will continue on another thread, context will be destroyed
    });

    return false;
}

void scheduler::schedule(std::list<coroutine*>& coros)
{
    for(coroutine* coro : coros)
    {
        schedule(coro);
    }
}

void scheduler::schedule(coroutine_weak_ptr coro)
{
    CORO_LOG("SCHED: scheduling corountine '", coro->name(), "'");

    // create new active context, if limit not reached yet
    {
        std::lock_guard<mutex> contexts_lock(_contexts_mutex);
        if (_active_contexts.size() < _max_allowed_running_coroutines)
        {
            CORO_LOG("SCHED: scheduling corountine, idle context exists, adding there");
            context_ptr ctx(new context(this));
            context* ctx_ptr = ctx.get();
            ctx->enqueue(coro);
            _active_contexts.push_back(std::move(ctx));

            _thread_pool.run([ctx_ptr]()
            {
                ctx_ptr->run();
            });

            return;
        }
    }

    // called from withing working context
    context* ctx = context::current_context();
    if (ctx && ctx->enqueue(std::move(coro)))
    {
        CORO_LOG("SCHED: scheduling corountine: adding to current context's list");
        context::current_context()->enqueue(std::move(coro));
    }
    else
    {
        CORO_LOG("SCHED: scheduling corountine: adding to global list");
        // put into global queue
        _global_queue.push(std::move(coro));
    }
}

void scheduler::go(coroutine_ptr&& coro)
{
     coroutine_weak_ptr coro_weak = coro.get();
    {
        std::lock_guard<mutex> lock(_coroutines_mutex);

        _coroutines.emplace_back(std::move(coro));
        _max_active_coroutines = std::max(_coroutines.size(), _max_active_coroutines);
    }

    schedule(coro_weak);
}

} // namespace coroutines
