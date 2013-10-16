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
        _running_processors = max_running_coroutines; // initially assume all runningm until they threads start and report back
        for(unsigned i = 0; i < max_running_coroutines; i++)
        {
            _processors.emplace_back(new processor(*this));
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
    std::cerr << "              active processors: " << _processors.size() << std::endl;
    std::cerr << "      running active processors: " << _running_processors << std::endl;
    std::cerr << "             blocked processors: " << _blocked_processors.size() << std::endl;
    std::cerr << "     running blocked processors: " << _running_blocked_processors << std::endl;

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
            assert(_running_blocked_processors > 0);
            auto it = find_ptr(_blocked_processors, pc);
            assert(it != _blocked_processors.end());

            std::swap(*it, _blocked_processors[_running_blocked_processors]);
            _running_blocked_processors--;

            // garbage-collect if there is too many idle blocked
            void remove_inactive_blocked_processors();
        }
        else
        {
            assert(_running_processors > 0);
            auto it = find_ptr(_processors, pc);
            assert(it != _processors.end());

            // TODO try to steal something
            _running_processors--;
            std::swap(*it, _processors[_running_processors]);
        }
    }
}

void scheduler::processor_blocked(processor_weak_ptr pc, std::vector<coroutine_weak_ptr>& queue)
{
    CORO_LOG("SCHED: processor ", pc, " blocked");

    // active/blocked before: [ A ] [ X ] [ A ] [ A ] [ i ] [ i ]  / [ B ] [ i ] [ i ]
    // swap with last active: [ A ] [ A ] [ A ] [ X ] [ i ] [ i ]  / [ B ] [ i ] [ i ]
    // swap with first inactive blocked:
    //                        [ A ] [ A ] [ A ] [ i ] [ i ] [ i ]  / [ B ] [ X ] [ i ]

    // move to blocked, schedule coroutines
    {
        std::lock_guard<mutex> lock(_processors_mutex);

        assert(_running_processors > 0);
        auto it = find_ptr(_processors, pc);
        assert(it != _processors.end());

        // swap with the last active
        _running_processors--;
        std::swap(*it, _processors[_running_processors]);

        // make sure there is at least one inactive blocked
        if(_running_blocked_processors == _blocked_processors.size())
        {
            // is there an inactive non-blocked processor we can re-use?
            if (_processors.size() > _max_allowed_running_coros && _processors.size() > (_running_processors+1))
            {
                _blocked_processors.emplace_back(std::move(_processors.back()));
                _processors.pop_back();
            }
            else
            {
                _blocked_processors.emplace_back(new processor(*this));
            }
        }

        // swap with the first inactive blocked
        std::swap(_processors[_running_processors], _blocked_processors[_running_blocked_processors]);
        _running_blocked_processors++;
    }

    // the procesor will now continue in blocked state

    // schedule coroutines
    schedule(queue);
}

void scheduler::processor_unblocked(processor_weak_ptr pc)
{
    std::lock_guard<mutex> lock(_processors_mutex);

    assert(_running_blocked_processors > 0);
    auto it = find_ptr(_blocked_processors, pc);
    assert(it != _blocked_processors.end());


    std::swap(*it, _blocked_processors[_running_blocked_processors]);

    // if there is no inactive in _processors, remove ours from blocked and move to _processors
    if(_running_processors == _processors.size())
    {
        std::swap(_blocked_processors[_running_blocked_processors], _blocked_processors.back());
        _processors.emplace_back(std::move(_blocked_processors.back()));
        _blocked_processors.pop_back();
    }
    else
    {
         std::swap(_blocked_processors[_running_blocked_processors], _processors[_running_processors]);
    }
    _running_blocked_processors--;
    _running_processors++;


    remove_inactive_blocked_processors();
}


void scheduler::remove_inactive_blocked_processors()
{
    // assumption: _processors_mutex is held

    // leave _active_blocked_processors idle
    unsigned to_leave = _running_blocked_processors * 2;
    if (_blocked_processors.size() > to_leave)
    {
        _blocked_processors.resize(to_leave);
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
    // activate new processor if limit not reached yet
    if(_running_processors < _processors.size())
    {
         CORO_LOG("SCHED: scheduling corountine, idle processor exists, adding there");
        _processors[_running_processors]->enqueue(coro);
        _running_processors++;
        return;
    }

    // called from withing working processor - add it there
    processor* current = processor::current_processor();
    if (current)
    {
        CORO_LOG("SCHED: scheduling corountine, adding to current context");
        current->enqueue(coro);
    }
    else
    {
        CORO_LOG("SCHED: scheduling corountine, adding to any context");
        // add to random
        _processors[random_index()]->enqueue(coro);
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
