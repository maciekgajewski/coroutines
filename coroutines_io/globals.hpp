// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_IO_GLOBALS_HPP
#define COROUTINES_IO_GLOBALS_HPP

#include <string>

namespace coroutines
{

class service;

void set_service(service* s);
service* get_service();
service& get_service_check(); // asserts service != null

void throw_errno();
void throw_errno(const std::string& what);

enum fd_events
{
    FD_READABLE = 0x01,
    FD_WRITABLE = 0x02
};


}

#endif

