// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_IO_SERVICE_HPP
#define COROUTINES_IO_SERVICE_HPP

#include "coroutines/scheduler.hpp"
#include "coroutines_io/detail/poller.hpp"
#include "coroutines_io/globals.hpp"

#include <system_error>

namespace coroutines {

class io_scheduler
{
public:

    io_scheduler(scheduler& sched);
    ~io_scheduler();

    scheduler& get_scheduler() { return _scheduler; }

    // services provided
    void wait_for_writable(int fd, const channel_writer<std::error_code>& writer);
    void wait_for_readable(int fd, const channel_writer<std::error_code>& writer);

    void start();
    void stop();

private:

    struct command
    {
        int fd;
        fd_events events;
        channel_writer<std::error_code> writer;
    };

    void loop();
    scheduler& _scheduler;
    detail::poller _poller;
    channel_writer<command> _command_writer;
    channel_reader<command> _command_reader;
};

}

#endif
