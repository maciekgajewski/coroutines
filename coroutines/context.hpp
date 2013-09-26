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
    context(context&& o);

    // adds coro to queue
    void enqueue(coroutine&& c);

    // version for multiple coros
    void enqueue(std::list<coroutine>& cs);

    // thread rountine
    void run();

    void swap(context& o);

    // returns pointer to context serving current thread
    static context* current_context();

private:

    thread_safe_queue<coroutine> _queue;
    coroutine_scheduler* _parent;
};

}

#endif // COROUTINES_CONTEXT_HPP
