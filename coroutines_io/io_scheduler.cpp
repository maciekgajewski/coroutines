// Copyright (c) 2013 Maciej Gajewski

#include "coroutines_io/io_scheduler.hpp"
#include "coroutines_io/globals.hpp"
#include "coroutines/globals.hpp"

#include "coroutines_io/detail/poller.hpp"

//#define CORO_LOGGING
#include "coroutines/logging.hpp"

#include <unordered_map>
#include <iostream>

#include <sys/socket.h>

namespace coroutines {

io_scheduler::io_scheduler(scheduler& sched)
    : _scheduler(sched)
{
}

io_scheduler::~io_scheduler()
{
}

void io_scheduler::wait_for_writable(int fd, const channel_writer<std::error_code>& writer)
{
    _command_writer.put(command{fd, FD_WRITABLE, writer});
    _poller.wake();
}

void io_scheduler::wait_for_readable(int fd, const channel_writer<std::error_code>& writer)
{
    _command_writer.put(command{fd, FD_READABLE, writer});
    _poller.wake();
}

void io_scheduler::start()
{
    auto pair = _scheduler.make_channel<command>(256, "io_scheduler command channel");
    _command_writer = std::move(pair.writer);
    _command_reader = std::move(pair.reader);

    _scheduler.go("service loop", [this](){ loop(); });
}

void io_scheduler::stop()
{
    _command_writer.close();
}

void io_scheduler::loop()
{
    std::unordered_map<std::uint64_t, command> commands;
    std::uint64_t counter = 0;
    std::vector<std::uint64_t> keys;

    for(;;)
    {
        command cmd;

        // if no pending commands, block on reader
        if (commands.empty())
        {
            CORO_LOG("SERV: blocking on commands");
            try
            {
                cmd = _command_reader.get();
            }
            catch(const channel_closed&)
            {
                CORO_LOG("SERV: blocking on commands interrupted");
                throw;
            }
            _poller.add_fd(cmd.fd, cmd.events, counter);
            commands.insert(std::make_pair(counter++, cmd));
        }

        // read all events
        while(_command_reader.try_get(cmd))
        {
            _poller.add_fd(cmd.fd, cmd.events, counter);
            commands.insert(std::make_pair(counter++, cmd));
        }

        CORO_LOG("SERV: polling, ", commands.size(), " sockets pending");

        // poll!
        keys.clear();
        _poller.wait(keys);

        CORO_LOG("SERV: ", keys.size(), " events ready");

        // serve events
        for(std::uint64_t key : keys)
        {
            auto it = commands.find(key);
            assert(it != commands.end());

            // get error
            std::error_code ec;
            int err = 0;
            socklen_t err_len = sizeof(err);
            int r = ::getsockopt(it->second.fd, SOL_SOCKET, SO_ERROR, &err, &err_len);
            if (r == 0)
            {
                ec = std::error_code(err, std::system_category());
            }

            it->second.writer.put(ec);
            commands.erase(it);
            _poller.remove_fd(it->second.fd);
        }

    }


}

}
