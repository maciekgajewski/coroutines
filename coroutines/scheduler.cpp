// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "coroutines/scheduler.hpp"
#include "coroutines/algorithm.hpp"

//#define CORO_LOGGING
#include "coroutines/logging.hpp"

#include <cassert>
#include <iostream>
#include <cstdlib>

namespace coroutines {

scheduler::scheduler(unsigned max_running_coroutines)
    : _max_allowed_running_coros(max_running_coroutines)
    , _random_generator(std::random_device()())
{
    assert(max_running_coroutines > 0);

    // setup
    {
        std::lock_guard<mutex> lock(_processors_mutex);
        for(unsigned i = 0; i < max_running_coroutines; i++)
        {
            _processors.insert(processor_ptr(new processor(*this)), PROCESSOR_STATE_IDLE);
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
    std::cerr << "             running processors: " << _processors.count_category(PROCESSOR_STATE_RUNNING) << std::endl;
    std::cerr << "             blocked processors: " << _processors.count_category(PROCESSOR_STATE_BLOCKED) << std::endl;
    std::cerr << "                idle processors: " << _processors.count_category(PROCESSOR_STATE_IDLE) << std::endl;

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

void scheduler::processor_idle(processor_weak_ptr pc)
{
    CORO_LOG("SCHED: processor ", pc, " finished");

    {
        std::lock_guard<mutex> lock(_processors_mutex);

        _processors.set_category(pc, PROCESSOR_STATE_IDLE);

        // try to steal some jobs
        unsigned running = _processors.count_category(PROCESSOR_STATE_RUNNING);

        std::vector<coroutine_weak_ptr> stolen;
        if (running > 0)
        {
            std::uniform_int_distribution<unsigned> dist(0, running-1);

            int idx = dist(_random_generator);
            for(unsigned i = 0; i < running; i++)
            {
                _processors.get_nth(PROCESSOR_STATE_RUNNING, idx)->steal(stolen);
                if (stolen.empty())
                {
                    idx = (idx + 1) % running; // try another one until loop finishes
                }
                else
                {
                    break;
                }
            }
        }

        // if stealing successful - reactivate
        if (!stolen.empty())
        {
            pc->enqueue(stolen);
            _processors.set_category(pc, PROCESSOR_STATE_RUNNING);
        }
        else
        {
            // garbage-collect if there is too many idle blocked
            remove_inactive_processors();
        }
    }
}

void scheduler::processor_blocked(processor_weak_ptr pc, std::vector<coroutine_weak_ptr>& queue)
{
    CORO_LOG("SCHED: processor ", pc, " blocked");

    // move to blocked, schedule coroutines
    {
        std::lock_guard<mutex> lock(_processors_mutex);

        _processors.set_category(pc, PROCESSOR_STATE_BLOCKED);

        // make sure there is enough idle processors to pickup the jobs
        while(_processors.count_category(PROCESSOR_STATE_RUNNING) + _processors.count_category(PROCESSOR_STATE_IDLE)
            < _max_allowed_running_coros)
        {
            _processors.insert(processor_ptr(new processor(*this)), PROCESSOR_STATE_IDLE);
        }
    }
    // the procesor will now continue in blocked state

    // schedule coroutines
    schedule(queue);
}

void scheduler::processor_unblocked(processor_weak_ptr pc)
{
    std::lock_guard<mutex> lock(_processors_mutex);

    _processors.set_category(pc, PROCESSOR_STATE_RUNNING);

    remove_inactive_processors();
}


void scheduler::remove_inactive_processors()
{
    // assumption: _processors_mutex is held

    // we kan keep up to _max_allowed_running_coros of stand-by threads
    while(_processors.count_category(PROCESSOR_STATE_IDLE) > _max_allowed_running_coros)
    {
        _processors.remove(_processors.get_nth(PROCESSOR_STATE_IDLE, 0));
    }
}

// returns uniform random number between 0 and _max_allowed_running_coros
unsigned scheduler::random_index()
{
    std::uniform_int_distribution<unsigned> dist(0, _max_allowed_running_coros-1);
    return dist(_random_generator);
}


void scheduler::schedule(std::vector<coroutine_weak_ptr>& coros)
{
    for(coroutine_weak_ptr coro : coros)
    {
        schedule(coro);
    }
}

void scheduler::schedule(coroutine_weak_ptr coro)
{
    CORO_LOG("SCHED: scheduling corountine '", coro->name(), "'");

    std::lock_guard<mutex> lock(_processors_mutex);

    unsigned running = _processors.count_category(PROCESSOR_STATE_RUNNING);
    processor* current = processor::current_processor();

    if(running < _max_allowed_running_coros)
    {
         CORO_LOG("SCHED: scheduling corountine, idle processor exists, adding there");
         auto p = _processors.get_nth(PROCESSOR_STATE_IDLE, 0);
         assert(p);
         p->enqueue(coro);
         _processors.set_category(p, PROCESSOR_STATE_RUNNING);
    }

    // called from withing working processor - add it there,
    // but only we don't have too much running
    else if (current && running == _max_allowed_running_coros)
    {
        CORO_LOG("SCHED: scheduling corountine, adding to current context");
        current->enqueue(coro);
    }
    else
    {
        CORO_LOG("SCHED: scheduling corountine, adding to any context");
        // add to random
        _processors.get_nth(PROCESSOR_STATE_RUNNING, random_index())->enqueue(coro);
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
