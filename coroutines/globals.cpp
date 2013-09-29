// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "globals.hpp"

namespace coroutines
{

static scheduler* __scheduler = nullptr;

void set_scheduler(scheduler* sched)
{
    __scheduler = sched;
}

scheduler* get_scheduler()
{
    return __scheduler;
}

scheduler& get_scheduler_check()
{
    assert(__scheduler);
    return *__scheduler;
}

}
