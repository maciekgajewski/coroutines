// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_IO_DETAIL_POLLER_HPP
#define COROUTINES_IO_DETAIL_POLLER_HPP

#include "coroutines/channel.hpp"
#include "coroutines_io/globals.hpp"

#include <system_error>
#include <vector>

namespace coroutines {

namespace detail {


// wrapper for epoll
class poller
{
public:

    poller();
    ~poller();

    // not thread safe, has to be called between calls to wait();
    void add_fd(int fd, fd_events e, std::uint64_t key);
    void remove_fd(int fd);

    // will block until one of the fd's bcomes active, ro wakeup() is called
    // filles 'keys' with activated descriptors
    void wait(std::vector<std::uint64_t>& keys);

    // interrupts wait().
    void wake();

private:

    static int to_epoll_events(fd_events e);

    int _event_fd = -1;
    int _epoll = -1;
};

}}

#endif
