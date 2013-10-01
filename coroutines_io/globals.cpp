#include "globals.hpp"

#include <cassert>
#include <system_error>

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

void throw_errno()
{
    throw std::system_error(errno, std::system_category());
}

void throw_errno(const std::string& what)
{
    throw std::system_error(errno, std::system_category(), what);
}


}
