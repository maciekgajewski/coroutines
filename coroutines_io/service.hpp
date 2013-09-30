// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_IO_SERVICE_HPP
#define COROUTINES_IO_SERVICE_HPP

#include <boost/asio/io_service.hpp>

#include "coroutines/scheduler.hpp"

namespace coroutines {

class service
{
public:

    service(scheduler& sched);
    ~service();

    scheduler& get_scheduler() { return _scheduler; }
    boost::asio::io_service& get_io_service() { return _io_service; }

    void start();
    void stop();

private:

    void loop();

    scheduler& _scheduler;
    boost::asio::io_service _io_service;
    bool _run = false;
};

}

#endif
