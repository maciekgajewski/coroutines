// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_IO_GLOBALS_HPP
#define COROUTINES_IO_GLOBALS_HPP

#include <string>

namespace coroutines
{

class io_scheduler;

void set_io_scheduler(io_scheduler* s);
io_scheduler* get_io_scheduler();
io_scheduler& get_io_scheduler_check(); // asserts io_scheduler != null

void throw_errno();
void throw_errno(const std::string& what);

enum fd_events
{
    FD_READABLE = 0x01,
    FD_WRITABLE = 0x02
};


}

#endif

