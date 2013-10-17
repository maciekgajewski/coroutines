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

    assert(!_running);
    stop_and_join();
}

void processor::enqueue(coroutine_weak_ptr coro)
{
    {
        std::lock_guard<mutex> lock(_queue_mutex);
        _queue.push_back(std::move(coro));
    }

    wakeup();
}

void processor::enqueue(std::vector<coroutine_weak_ptr>& coros)
{
    {
        std::lock_guard<mutex> lock(_queue_mutex);

        for( coroutine_weak_ptr& coro : coros)
        {
            _queue.push_back(std::move(coro));
        }
        coros.clear();
    }

    wakeup();
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

void processor::stop_and_join()
{
    {
        std::lock_guard<mutex> lock(_runnng_mutex);
        _stopped = true;
        assert(!_running);
        _running_cv.notify_one();
    }
    _thread.join();
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
            if (!_queue.empty())
            {
                coro = std::move(_queue.front());
                _queue.pop_front();
            }
        }

        if (coro)
        {
            // execute
            CORO_LOG("PROC=", this, " : will run coro '", coro->name(), "'");
            coro->run();
        }
        else
        {
            // wait for wakeup
            _runnng_mutex.lock();
            _running = false;
            _runnng_mutex.unlock();

            CORO_LOG("PROC=", this, " idle");
            _scheduler.processor_idle(this); // mutex must be unlocked here, as enqueueu->wakup may be called

            std::lock_guard<mutex> lock(_runnng_mutex);
            _running_cv.wait(_runnng_mutex, [this]() { return _running || _stopped; });

            if (_stopped)
                return;
        }
    }
}

void processor::wakeup()
{
    std::lock_guard<mutex> lock(_runnng_mutex);
    if (!_running)
    {
        _running = true;
        _running_cv.notify_one();
    }
}

} // namespace coroutines
