// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_IO_GLOBALS_HPP
#define COROUTINES_IO_GLOBALS_HPP

namespace coroutines
{

class service;

void set_service(service* s);
service* get_service();
service& get_service_check(); // asserts service != null

void throw_errno();

enum fd_events
{
    FD_READABLE = 0x00,
    FD_WRITABLE = 0x01
};


}

#endif

