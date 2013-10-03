// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_CONTEXT_HPP
#define COROUTINES_CONTEXT_HPP

#include "coroutine.hpp"
#include "thread_safe_queue.hpp"

namespace coroutines {

class scheduler;

// running thread state
// a resource which needs to eb acquired to run coroutine, which exists in limited number.
class context
{
public:
    context(scheduler* parent);

    // adds coro to queue
    void enqueue(coroutine_weak_ptr c);

    // version for multiple coros
    void enqueue(std::list<coroutine_weak_ptr>& cs);

    // takes half of the queue, returns number of the coros taken
    unsigned steal(std::list<coroutine_weak_ptr>& out);

    // thread rountine
    void run();

    // returns pointer to context serving current thread
    static context* current_context();

    // moves current context into blocking mode. Used by croutines calling into blocking syscall
    void block(const std::string& checkpoint_name);

    // mattempts to move the context bck from blocking mode. may preemt calling coroutine
    void unblock(const std::string& checkpoint_name);

private:

    thread_safe_queue<coroutine_weak_ptr> _queue;
    scheduler* _parent;
    bool _blocked = false;
};

typedef std::unique_ptr<context> context_ptr;

}

#endif // COROUTINES_CONTEXT_HPP
