// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_IO_GLOBALS_HPP
#define COROUTINES_IO_GLOBALS_HPP

#include "service.hpp"

namespace coroutines_io
{

void set_service(service* s);
service* get_service();
service& get_service_check(); // asserts service != null

}

#endif

