#include "globals.hpp"

#include <cassert>

namespace coroutines
{

static service* __service = nullptr;

void set_service(service* s)
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
    return *__service;
}


}
