// Copyright (c) 2013 Maciej Gajewski

#include "service.hpp"

namespace coroutines {

service::service(scheduler& sched)
    : _scheduler(sched)
{
}

service::~service()
{
}

void service::start()
{
    _run = true;
    _scheduler.go("io service loop", [this](){ loop(); });
}

void service::stop()
{
    _run = false;
}

void service::loop()
{
    // main loop
    while(_run)
    {
        std::cout << "before run_one" << std::endl;
        _io_service.run_one();
        std::cout << "after run_one" << std::endl;
    }
}


}
