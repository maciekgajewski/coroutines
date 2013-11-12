// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_TESTS_FIXTURES_HPP
#define COROUTINES_TESTS_FIXTURES_HPP

#include "coroutines/globals.hpp"

namespace coroutines { namespace tests {

// simple fixture creating a 4-thread scheduler
class fixture
{
public:

    fixture()
    : _sched(4)
    {
        set_scheduler(&_sched);
    }

    ~fixture()
    {
        _sched.wait();
        set_scheduler(nullptr);
    }

protected:

    void wait_for_completion()
    {
        _sched.wait();
    }

private:

    scheduler _sched;
};

} }

#endif
