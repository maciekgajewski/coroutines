// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_IO_SERVICE_HPP
#define COROUTINES_IO_SERVICE_HPP

#include "coroutines/scheduler.hpp"

#include <system_error>

namespace coroutines {

class service
{
public:

    service(scheduler& sched);
    ~service();

    scheduler& get_scheduler() { return _scheduler; }

    // servuices provided
    void wait_for_writable(int socket, const channel_writer<std::error_code>& writer);
    void wait_for_readable(int socket, const channel_writer<std::error_code>& writer);

private:

    struct command
    {
        int fd;
        int events;
        channel_writer<std::error_code> writer;
    };


    void loop();

    scheduler& _scheduler;

    channel_writer<command> _command_writer;
    channel_reader<command> _command_reader;

    int _event_fd = -1;

};

}

#endif
