// Copyright (c) 2013 Maciej Gajewski

#ifndef COROUTINES_CHANNEL_CLOSED_HPP
#define COROUTINES_CHANNEL_CLOSED_HPP

#include <stdexcept>

namespace coroutines {

// Exception thrown when channel is closed
struct channel_closed : public std::exception
{
    virtual const char* what() const noexcept { return "channel closed"; }
};

}

#endif
