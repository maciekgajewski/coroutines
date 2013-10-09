// Copyright (c) 2013 Maciej Gajewski
#include "coroutines_io/detail/poller.hpp"
#include "coroutines/algorithm.hpp"
#include "coroutines_io/globals.hpp"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <signal.h>
#include <unistd.h>

#include <limits>
#include <iostream>

namespace coroutines { namespace detail {

static const std::uint64_t EVENTFD_KEY = std::numeric_limits<std::uint64_t>::max();

poller::poller()
{
    _event_fd = ::eventfd(0, EFD_NONBLOCK);
    if (_event_fd < 0)
        throw_errno();

    _epoll = ::epoll_create(10);
    if (_epoll < 0)
        throw_errno();

    // add event to epoll
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.u64 = EVENTFD_KEY;
    int r = ::epoll_ctl(_epoll, EPOLL_CTL_ADD, _event_fd,  &ev);
    if (r < 0)
        throw_errno();
}

poller::~poller()
{
    ::close(_event_fd);
    ::close(_epoll);
}

void poller::add_fd(int fd, fd_events e, std::uint64_t key)
{
    // add to epoll
    epoll_event ev;
    ev.events = to_epoll_events(e) | EPOLLERR | EPOLLHUP /*| EPOLLET*/;
    ev.data.u64 = key;

    int r = ::epoll_ctl(_epoll, EPOLL_CTL_ADD, fd, &ev);
    if (r < 0)
        throw_errno("poller::add_fd");

//    std::cout << "POLLER: fd " << fd << " added to epoll with flags " << std::hex << ev.events << ", fd_Events=" << std::dec << int(e) << std::endl;
}

void poller::remove_fd(int fd)
{
    int r = ::epoll_ctl(_epoll, EPOLL_CTL_DEL, fd, nullptr);
    if (r < 0)
        throw_errno("poller::remove_fd");
}

void poller::wait(std::vector<std::uint64_t>& keys)
{
    static const unsigned EPOLL_BUFFER = 256;
    epoll_event events[EPOLL_BUFFER];

    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGTRAP);

    int r = ::epoll_pwait(_epoll, events, EPOLL_BUFFER, -1, &sigs);

//    std::cout << "POLLER: woken up with " << r << "events ready" << std::endl;

    if (r < 0 && errno != EINTR)
        throw_errno();

    for(int i = 0; i < r; i++)
    {
        std::uint64_t key = events[i].data.u64;
        if (key == EVENTFD_KEY)
        {
            // re-arm eventfd
            std::uint64_t v = 1;
            if (::read(_event_fd, &v, sizeof(v)) < 0 )
                throw_errno();
        }
        else
        {
            keys.push_back(key);
        }
    }
}

void poller::wake()
{
    std::uint64_t v = 1;
    if (::write(_event_fd, &v, sizeof(v)) < 0 )
        throw_errno();
}

int poller::to_epoll_events(fd_events e)
{
    int result = 0;

    if (e & FD_READABLE) result |= EPOLLIN;
    if (e & FD_WRITABLE) result |= EPOLLOUT;

    return result;
}



}}
