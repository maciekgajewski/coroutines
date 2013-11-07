// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "coroutines/processor.hpp"
#include "coroutines/scheduler.hpp"
#include "profiling/profiling.hpp"

//#define CORO_LOGGING
#include "coroutines/logging.hpp"

#include <mutex>
#include <algorithm>

namespace coroutines {

static thread_local processor* __current_processor= nullptr;

processor::processor(scheduler& sched)
    : _scheduler(sched)
    , _queue_mutex("processor queue mutex")
    , _thread([this]() { routine(); })
{
}

processor::~processor()
{
    CORO_LOG("PROC=", this, " destroyed");

    _thread.join();
}

template<typename InputIterator>
bool processor::enqueue(InputIterator first, InputIterator last)
{
    {
        std::lock_guard<mutex> lock(_queue_mutex);

        if (_stopped || _blocked)
            return false;

        _queue.insert(_queue.end(), first, last);
    }

    _cv.notify_one();
    return true;
}

// force instantiation for std::vector
template bool processor::enqueue<std::vector<coroutine_weak_ptr>::iterator>(std::vector<coroutine_weak_ptr>::iterator, std::vector<coroutine_weak_ptr>::iterator);
// and for raw pointers
template bool processor::enqueue<coroutine_weak_ptr*>(coroutine_weak_ptr*, coroutine_weak_ptr*);

bool processor::enqueue(coroutine_weak_ptr coro)
{
    return enqueue(&coro, &coro + 1);
}

bool processor::stop()
{
    std::lock_guard<mutex> lock(_queue_mutex);
    _stopped = true;
    _cv.notify_one();

    return _queue.empty() || _executing;
}

bool processor::stop_if_idle()
{
    std::lock_guard<mutex> lock(_queue_mutex);
    if (_queue.empty() && !_executing)
    {
        _stopped = true;
        _cv.notify_one();
        return true;
    }
    return false;
}


void processor::steal(std::vector<coroutine_weak_ptr>& out)
{
    {
        std::lock_guard<mutex> lock(_queue_mutex);

        unsigned to_steal = _queue.size() / 2; // rounds down
        if (to_steal > 0)
        {
            unsigned to_leave = _queue.size() - to_steal;
            auto it = _queue.begin();
            std::advance(it, to_leave);

            out.reserve(out.size() + to_steal);
            std::for_each(it, _queue.end(), [&out](coroutine_weak_ptr& coro)
            {
                out.push_back(std::move(coro));
            });
            _queue.resize(to_leave);
        }
    }
}

unsigned processor::queue_size()
{
    std::lock_guard<mutex> lock(_queue_mutex);
    return _queue.size() + _executing;
}

processor* processor::current_processor()
{
    return __current_processor;
}

void processor::routine()
{
    CORO_PROF("processor", this, "routine started");

    __current_processor = this;
    struct scope_exit { ~scope_exit() { __current_processor = nullptr; } } exit;

    for(;;)
    {
        // am I short on jobs?
        bool no_jobs = false;
        {
            std::lock_guard<mutex> lock(_queue_mutex);
            _executing = false;
            no_jobs = _queue.empty();
        }
        if(no_jobs)
            _scheduler.processor_idle(this); // ask for more

        // take coro from queue
        coroutine_weak_ptr coro = nullptr;
        {
            std::lock_guard<mutex> lock(_queue_mutex);

            _cv.wait(
                _queue_mutex,
                [this](){ return _stopped || !_queue.empty(); });

            if (_queue.empty())
            {
                assert(_stopped);
                CORO_LOG("PROC=", this, " : Stopped, and queue empty. Stopping");
                return;
            }
            else
            {
                coro = _queue.front();
                _queue.pop_front();
            }
            _executing = true;
        }

        // execute
        CORO_LOG("PROC=", this, " : will run coro '", coro->name(), "'");
        coro->run();
    }

    CORO_PROF("processor", this, "routine finished");
}

} // namespace coroutines
