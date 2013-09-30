// Copyright (c) 2013 Maciej Gajewski

#include "service.hpp"
#include "globals.hpp"

#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <unordered_map>

namespace coroutines {

service::service(scheduler& sched)
    : _scheduler(sched)
{
    _event_fd = ::eventfd(0, 0);
    if (_event_fd < 0)
        throw_errno();

    auto pair = _scheduler.make_channel<command>(10);
    _command_writer = std::move(pair.writer);
    _command_reader = std::move(pair.reader);

    _scheduler.go("service loop", [this](){ loop(); });
}

service::~service()
{
}

void service::wait_for_writable(int socket, const channel_writer<std::error_code>& writer)
{
}

void service::wait_for_readable(int socket, const channel_writer<std::error_code>& writer)
{
}

void service::loop()
{
    int epoll = ::epoll_create(10);
    if (epoll < 0)
        throw_errno();

    // add event to epoll
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.u32 = 0;
    int r = ::epoll_ctl(
        epoll,
        EPOLL_CTL_ADD,
        _event_fd,
        &ev);
    if (r < 0)
        throw_errno();



    std::unordered_map<std::uint32_t, command> commands;
    std::uint32_t counter = 1;

    for(;;)
    {
        command cmd;

        //read all events
        while(_command_reader.try_get(cmd))
        {
            commands.insert(std::make_pair(counter, cmd));

            // add to epoll
            epoll_event ev;
            ev.events = cmd.events;
            ev.data.u32 = counter++;

            int r = ::epoll_ctl(
                epoll,
                EPOLL_CTL_ADD,
                cmd.fd,
                &ev);
            if (r < 0)
                throw_errno();
        }

        // poll!
        std::cout << "EPOLL: will wait, evnets observed: " << commands.size() << std::endl;
        static const unsigned EPOLL_BUFFER = 256;
        epoll_event events[EPOLL_BUFFER];

        int r = ::epoll_wait(epoll, events, EPOLL_BUFFER, -1);
        if (r < 0)
            throw_errno();
        std::cout << "EPOLL: " << r << " events ready" << std::endl;

        // serve events
        for(int i = 0; i < r; i++)
        {
            auto it = commands.find(events[i].data.u32);
            assert(it != commands.end());
            std::cout << "fd=" << it->second.fd << ", events: " << events[i].events << std::endl;
            it->second.writer.put(std::error_code());
            commands.erase(it);
        }

    }


}


}
