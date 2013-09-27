// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "coroutine.hpp"
#include "context.hpp"

#include "monitor.hpp"

namespace coroutines {

monitor::monitor()
{
}

monitor::~monitor()
{
    //std::cout << "MONITOR: this=" << this << " deleting" << std::endl;
    assert(_waiting.empty());
}

void monitor::wait(epilogue_type epilogue)
{
    coroutine* coro = coroutine::current_corutine();
    assert(coro);

    //std::cout << "MONITOR: this=" << this << " '" << coro->name() << "' will wait" << std::endl;

    coro->yield([this, epilogue](coroutine_ptr& coro)
    {
        //std::cout << "MONITOR: this=" << this << " '" << coro->name() << "' added to queue" << std::endl;
        _waiting.push(std::move(coro));
        if (epilogue)
            epilogue();
    });
}

void monitor::wake_all()
{
    context* ctx = context::current_context();
    assert(ctx);

    std::list<coroutine_ptr> waiting;
    _waiting.get_all(waiting);

    //std::cout << "MONITOR: waking up " << waiting.size() << " coroutines" << std::endl;

    ctx->enqueue(waiting);
}

void monitor::wake_one()
{
    context* ctx = context::current_context();
    assert(ctx);

    //std::cout << "MONITOR: this=" << this << " will wake one. q contains: '" << _waiting.size() << std::endl;
    coroutine_ptr waiting;
    bool r = _waiting.pop(waiting);

    if (r)
    {
        //std::cout << "MONITOR: this=" << this << " waking up one coroutine ('" << waiting->name()
        //    << "'), " << _waiting.size() << " left in q" << std::endl;
        ctx->enqueue(std::move(waiting));
    }
//    else
//    {
//        std::cout << "MONITOR: this=" << this << " nothign to wake" << std::endl;
//    }
}

}
