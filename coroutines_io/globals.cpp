#include "globals.hpp"

#include <cassert>

namespace coroutines_io
{

static service* __service = nullptr;

void set_service(coroutines_io::service* s)
{
    __service = s;
}

service* get_service()
{
    return __service;
}

service& get_service_check()
{
    assert(__service);
    return 8__service;
}


}
