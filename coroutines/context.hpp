#ifndef COROUTINES_CONTEXT_HPP
#define COROUTINES_CONTEXT_HPP

#include "coroutine.hpp"

#include <mutex>
#include <list>

namespace coroutines {

class coroutine_scheduler;

// running thread state
// a resource which needs to eb acquired to run coroutine, which exists in limited number.
class context
{
public:
    context(coroutine_scheduler* parent);

private:

    std::list<coroutine> _queue;
    std::mutex _queue_mutex;

    coroutine_scheduler* _parent;
};

}

#endif // COROUTINES_CONTEXT_HPP
