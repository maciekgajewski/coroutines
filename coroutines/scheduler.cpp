// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "coroutines/scheduler.hpp"
#include "coroutines/algorithm.hpp"

//#define CORO_LOGGING
#include "coroutines/logging.hpp"

#include <cassert>
#include <iostream>
#include <cstdlib>

namespace coroutines {

scheduler::scheduler(unsigned active_processors)
    : _active_processors(active_processors)
    , _processors()
    , _processors_mutex("processor mutex")
    , _coroutines_mutex("coroutines mutex")
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
            // give him the global queue
            if (!_global_queue.empty())
            {
                pc->enqueue(_global_queue.begin(), _global_queue.end());
                _global_queue.clear();
            }

            // try to steal
            unsigned most_busy = _processors.most_busy_index(0, _active_processors);
            std::vector<coroutine_weak_ptr> stolen;
            _processors[most_busy].steal(stolen);
            // if stealing successful - reactivate
            if (!stolen.empty())
            {
                CORO_LOG("SCHED: stolen ", stolen.size(), " coros for proc=", pc, " from proc=", &_processors[most_busy]);
                pc->enqueue(stolen.begin(), stolen.end());
            }
        }
        // else: I don't care, you are in exile
    }
}

// returns uniform random number between 0 and _active_processors-1
unsigned scheduler::random_index()
{
    std::uniform_int_distribution<unsigned> dist(0, _active_processors-1);
    return dist(_random_generator);
}


void scheduler::schedule(coroutine_weak_ptr coro)
{
    schedule(&coro, &coro + 1);
}

template<typename InputIterator>
void scheduler::schedule(InputIterator first,  InputIterator last)
{
    if (first == last)
        return; // that was easy :)

    CORO_LOG("SCHED: scheduling ", std::distance(first, last), " corountines. First:  '", (*first)->name(), "'");

    std::lock_guard<mutex> lock(_processors_mutex);

    CORO_LOG("SCHED: scheduling corountine, will add to random processor");
    unsigned index = random_index();

    for(unsigned i = 0; i < _active_processors; ++i)
    {
        if (_processors[index].enqueue(first, last))
        {
            CORO_LOG("SCHED: scheduling corountines, added to proc=", &_processors[index], ", index=", index);
            return;
        }
        index = (index + 1) % _active_processors;
    }

    // total failure, add to global queue?
    CORO_LOG("SCHED: scheduling corountines, added to global queue");
    _global_queue.insert(_global_queue.end(), first, last);
}

template
void scheduler::schedule<std::vector<coroutine_weak_ptr>::iterator>(std::vector<coroutine_weak_ptr>::iterator, std::vector<coroutine_weak_ptr>::iterator);

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
