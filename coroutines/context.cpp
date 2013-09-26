#include "context.hpp"

namespace coroutines {

thread_local context* __current_context;

context::context(coroutine_scheduler* parent)
    : _parent(parent)
{
}

context::context(context&& o)
{
    swap(o);
}

void context::swap(context& o)
{
    std::swap(_queue, o._queue);
    std::swap(_parent, o._parent);
}


void context::enqueue(coroutine&& c)
{
    _queue.push(std::move(c));
}

void context::run()
{
    __current_context = this;
    struct scope_exit { ~scope_exit() { __current_context = nullptr; } } exit;

    for(;;)
    {
        coroutine current_coro;
        bool has_next = _queue.pop(current_coro);

        // run the coroutine
        if (has_next)
        {
            current_coro.run();
        }
        else
        {
            break;
        }

        // TODO take on any job from global queue
    }
}

}
