// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "coroutines/monitor.hpp"

#include "coroutines/coroutine.hpp"
#include "coroutines/context.hpp"
#include "coroutines/scheduler.hpp"

//#define CORO_LOGGING
#include "coroutines/logging.hpp"

namespace coroutines {

monitor::monitor(scheduler& sched)
    : _scheduler(sched)
{
}

monitor::~monitor()
{
    //std::cout << "MONITOR: this=" << this << " deleting" << std::endl;
    assert(_waiting.empty());
}

void monitor::wait(const std::string& checkopint_name, epilogue_type epilogue)
{
    coroutine* coro = coroutine::current_corutine();
    assert(coro);

    CORO_LOG("MONITOR: this=",  this, " '", coro->name(), "' will wait");

    coro->yield(checkopint_name, [this, epilogue](coroutine_weak_ptr coro)
    {
        CORO_LOG("MONITOR: this=", this, " '", coro->name(), "' added to queue");
        {
            std::lock_guard<mutex> lock(_waiting_mutex);
            _waiting.push_back(std::move(coro));
        }
        if (epilogue)
            epilogue();
    });
}

void monitor::wake_all()
{
    CORO_LOG("MONITOR: wake_all");

    std::vector<coroutine*> waiting;
    {
        std::lock_guard<mutex> lock(_waiting_mutex);
        _waiting.swap(waiting);
    }

    CORO_LOG("MONITOR: waking up ", waiting.size(), " coroutine(s)");

    _scheduler.schedule(waiting);
}

void monitor::wake_one()
{
    CORO_LOG("MONITOR: this=", this, " will wake one. q contains: '", _waiting.size())

    coroutine_weak_ptr waiting = nullptr;
    {
        std::lock_guard<mutex> lock(_waiting_mutex);
        if (!_waiting.empty())
        {
            waiting = _waiting.back();
            _waiting.pop_back();
        }
    }

    if (waiting)
    {
        CORO_LOG("MONITOR: this=", this, " waking up one coroutine ('", waiting->name(), "'), ", _waiting.size(), " left in q");
        _scheduler.schedule(std::move(waiting));
    }
    else
    {
        CORO_LOG("MONITOR: this=", this, " nothign to wake");
    }
}

}
