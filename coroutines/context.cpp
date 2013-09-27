// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "context.hpp"
#include "coroutine_scheduler.hpp"

namespace coroutines {

static thread_local context* __current_context;

context::context(coroutine_scheduler* parent)
    : _parent(parent)
{
}

context*context::current_context()
{
    return __current_context;
}


void context::enqueue(coroutine_ptr&& c)
{
    _queue.push(std::move(c));
}

void  context::enqueue(std::list<coroutine_ptr>& cs)
{
    _queue.push(cs);
}


void context::run()
{
    __current_context = this;
    struct scope_exit { ~scope_exit() { __current_context = nullptr; } } exit;

    for(;;)
    {
        coroutine_ptr current_coro;
        bool has_next = _queue.pop(current_coro);

        // run the coroutine
        if (has_next)
        {
            current_coro->run(current_coro);
        }
        else
        {
            // TODO try to steal from another running thread
            break;
        }

        // TODO take on any job from global queue
        std::list<coroutine_ptr> globals;
        _parent->get_all_from_global_queue(globals);
        _queue.push(globals);
    }
}

}
