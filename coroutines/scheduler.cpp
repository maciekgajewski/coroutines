// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "coroutines/scheduler.hpp"
#include "coroutines/algorithm.hpp"

#define CORO_LOGGING
#include "coroutines/logging.hpp"

#include <cassert>
#include <iostream>
#include <cstdlib>

namespace coroutines {

scheduler::scheduler(unsigned active_processors)
    : _active_processors(active_processors)
    , _processors(active_processors)
    , _random_generator(std::random_device()())
{
    assert(active_processors > 0);

    // setup
    {
        std::lock_guard<mutex> lock(_processors_mutex);
        for(unsigned i = 0; i < active_processors; i++)
        {
            _processors.emplace_back(*this);
        }
    }
}

scheduler::~scheduler()
{
    wait();
    {
        std::lock_guard<mutex> lock(_processors_mutex);
        _processors.stop_all();
    }
    CORO_LOG("SCHED: destroyed");
}

void scheduler::debug_dump()
{
    std::lock(_coroutines_mutex, _processors_mutex);

    std::cerr << "=========== scheduler debug dump ============" << std::endl;
    std::cerr << "          active coroutines now: " << _coroutines.size() << std::endl;
    std::cerr << "     max active coroutines seen: " << _max_active_coroutines << std::endl;
    std::cerr << "               no of processors: " << _processors.size();
    std::cerr << "       no of blocked processors: " << _blocked_processors;

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
    CORO_LOG("SCHED: waiting...");

    std::unique_lock<mutex> lock(_coroutines_mutex);
    _coro_cv.wait(lock, [this]() { return _coroutines.empty(); });

    CORO_LOG("SCHED: wait over");
}

void scheduler::coroutine_finished(coroutine* coro)
{
    CORO_LOG("SCHED: coro=", coro, " finished");

    std::lock_guard<mutex> lock(_coroutines_mutex);
    auto it = find_ptr(_coroutines, coro);
    assert(it != _coroutines.end());
    _coroutines.erase(it);

    if (_coroutines.empty())
    {
        _coro_cv.notify_all();
    }
}

void scheduler::processor_idle(processor* pc)
{
    CORO_LOG("SCHED: processor ", pc, " idle");

    {
        std::lock_guard<mutex> lock(_processors_mutex);

        unsigned index = _processors.index_of(pc);
        if (index < _active_processors)
        {
            // try to steal
            unsigned most_busy = _processors.most_busy_index(0, _active_processors);
            std::vector<coroutine_weak_ptr> stolen;
            _processors[most_busy].steal(stolen);
            // if stealing successful - reactivate
            if (!stolen.empty())
            {
                CORO_LOG("SCHED: stolend ", stolen.size(), " coros for processor ", pc);
                pc->enqueue(stolen);
            }
        }
        // else: I don't care, you are in exile
    }
}

void scheduler::processor_blocked(processor_weak_ptr pc, std::vector<coroutine_weak_ptr>& queue)
{
    CORO_LOG("SCHED: processor ", pc, " blocked");

    // move to blocked, schedule coroutines
    {
        std::lock_guard<mutex> lock(_processors_mutex);

        unsigned index = _processors.index_of(pc);

        if(index < _max_active_coroutines)
        {
            // try to enqueue the on one of the idle threads
            unsigned idle = 0;
            for(unsigned i = _max_active_coroutines; i < _processors.size(); i++)
            {
                processor& pi = _processors[i];
                if (pi.queue_size() == 0 && pi.enqueue(queue))
                {
                    idle = i;
                    break;
                }
            }
            // failed, create a new one
            if (idle == 0)
            {
                idle = _processors.size();
                _processors.insert(_active_processors, *this);
                bool success = _processors.back().enqueue(queue);
                assert(success);
            }
            // swap with the idle
            _processors.swap(index, idle);
            return;
        } // else I don't casre, you are in exile

        _blocked_processors++;
    }
    // the procesor will now continue in blocked state


    // schedule coroutines
    schedule(queue);
}

void scheduler::processor_unblocked(processor_weak_ptr pc)
{
    std::lock_guard<mutex> lock(_processors_mutex);

    unsigned index = _processors.index_of(pc);
    assert(index >= _max_active_coroutines);

    // so you want tyo return to active duty?
    unsigned least_busy = _processors.least_busy_index(0, _active_processors);

    if (_processors[least_busy].queue_size() == 0)
    {
        // ok, I'll give you a chance
        _processors.swap(least_busy, index);
    }

    _blocked_processors++;

    remove_inactive_processors();
}


void scheduler::remove_inactive_processors()
{
    while(_processors.size() > _active_processors*2 + _blocked_processors)
    {
        CORO_LOG("SCHED: processors: ", _processors.size(), ", blocked: ", _blocked_processors, ", cleaning up");
        if (_processors.back().stop())
        {
            _processors.pop_back();
        }
        else
        {
            break; // some task is running, we'll come for him the next time
        }
    }
}

// returns uniform random number between 0 and _max_allowed_running_coros
unsigned scheduler::random_index()
{
    std::uniform_int_distribution<unsigned> dist(0, _active_processors-1);
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

    processor* current = processor::current_processor();
    unsigned least_busy = _processors.least_busy_index(0, _active_processors);

    // if able to queue on current, do it
    if (current)
    {
        unsigned index_current = _processors.index_of(current);
        unsigned least_busy_qs = _processors[least_busy].queue_size();
        unsigned current_qs = current->queue_size();

        if (index_current < _active_processors && least_busy_qs >= (current_qs/2))
        {
            // schedule on current
            CORO_LOG("SCHED: scheduling corountine, adding to current context");
            bool s = current->enqueue(coro);
            assert(s);
            return;
        }
    }

    CORO_LOG("SCHED: scheduling corountine, adding to context #", least_busy);
    bool s = _processors[least_busy].enqueue(coro);
    assert(s);
}

void scheduler::go(coroutine_ptr&& coro)
{
    CORO_LOG("SCHED: go '", coro->name(), "'");
    coroutine_weak_ptr coro_weak = coro.get();
    {
        std::lock_guard<mutex> lock(_coroutines_mutex);

        _coroutines.emplace_back(std::move(coro));
        _max_active_coroutines = std::max(_coroutines.size(), _max_active_coroutines);
    }

    schedule(coro_weak);
}

} // namespace coroutines
