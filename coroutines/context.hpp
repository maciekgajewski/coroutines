// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_CONTEXT_HPP
#define COROUTINES_CONTEXT_HPP

#include "coroutine.hpp"
#include "thread_safe_queue.hpp"

namespace coroutines {

class coroutine_scheduler;

// running thread state
// a resource which needs to eb acquired to run coroutine, which exists in limited number.
class context
{
public:
    context(coroutine_scheduler* parent);

    // adds coro to queue
    void enqueue(coroutine_ptr&& c);

    // version for multiple coros
    void enqueue(std::list<coroutine_ptr>& cs);

    // thread rountine
    void run();

    // returns pointer to context serving current thread
    static context* current_context();

private:

    thread_safe_queue<coroutine_ptr> _queue;
    coroutine_scheduler* _parent;
};

typedef std::unique_ptr<context> context_ptr;

}

#endif // COROUTINES_CONTEXT_HPP
