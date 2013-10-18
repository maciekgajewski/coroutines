// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "coroutines/processor.hpp"
#include "coroutines/scheduler.hpp"

#define CORO_LOGGING
#include "coroutines/logging.hpp"

#include <mutex>
#include <algorithm>

namespace coroutines {

static thread_local processor* __current_processor= nullptr;

processor::processor(scheduler& sched)
    : _scheduler(sched)
    , _thread([this]() { routine(); })
{
}

processor::~processor()
{
    CORO_LOG("PROC=", this, " destroyed");

    _thread.join();
}

bool processor::enqueue(coroutine_weak_ptr coro)
{
    {
        std::lock_guard<mutex> lock(_queue_mutex);

        if (_stopped)
            return false;

        _queue.push_back(std::move(coro));
    }

    _cv.notify_one();
    return true;
}

bool processor::enqueue(std::vector<coroutine_weak_ptr>& coros)
{
    {
        std::lock_guard<mutex> lock(_queue_mutex);

        if (_stopped)
            return false;

        for( coroutine_weak_ptr& coro : coros)
        {
            _queue.push_back(std::move(coro));
        }
        coros.clear();
    }

    _cv.notify_one();
    return true;
}

bool processor::stop()
{
    std::lock_guard<mutex> lock(_queue_mutex);
    _stopped = true;
    _cv.notify_one();

    return _queue.empty();
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
    return _queue.size();
}

void processor::block()
{
    CORO_LOG("PROC=", this, " block");

    std::vector<coroutine_weak_ptr> queue;
    {
        std::lock_guard<mutex> lock(_queue_mutex);

        queue.reserve(_queue.size());
        std::copy(_queue.begin(), _queue.end(), std::back_inserter(queue));
        _queue.clear();
    }

    _scheduler.processor_blocked(this, queue);
}

void processor::unblock()
{
    CORO_LOG("PROC=", this, " unblock");

    _scheduler.processor_unblocked(this);
}

processor* processor::current_processor()
{
    return __current_processor;
}

void processor::routine()
{
    __current_processor = this;
    struct scope_exit { ~scope_exit() { __current_processor = nullptr; } } exit;

    for(;;)
    {
        // take coro from queue
        coroutine_weak_ptr coro = nullptr;
        {
            std::lock_guard<mutex> lock(_queue_mutex);

            if (!_queue.empty()) _queue.pop_front(); // remove stub from the queue

            if (_queue.empty()) _scheduler.processor_idle(this);

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
                coro = std::move(_queue.front()); // but leave stub in the queue
            }
        }

        // execute
        CORO_LOG("PROC=", this, " : will run coro '", coro->name(), "'");
        coro->run();
    }
}

} // namespace coroutines
