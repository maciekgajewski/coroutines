// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "context.hpp"
#include "coroutine_scheduler.hpp"

namespace coroutines {

static thread_local context* __current_context = nullptr;

context::context(coroutine_scheduler* parent)
    : _parent(parent)
{
}

context* context::current_context()
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
            std::list<coroutine_ptr> stolen;
            _parent->steal(stolen);
            if (stolen.empty())
                break;
            _queue.push(stolen);
        }

        std::list<coroutine_ptr> globals;
        _parent->get_all_from_global_queue(globals);
        _queue.push(globals);
    }
}


unsigned context::steal(std::list<coroutine_ptr>& out)
{
    unsigned result = 0;
    _queue.perform([&result, &out](std::list<coroutine_ptr>& queue)
    {
        unsigned all = queue.size();
        result = all/2;

        auto it = queue.begin();
        std::advance(it, all-result);
        out.splice(out.end(), queue, it, queue.end());
    });

    return result;
}


}
