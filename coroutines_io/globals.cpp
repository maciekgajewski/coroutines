#include "globals.hpp"

#include <cassert>
#include <system_error>

namespace coroutines
{

static io_scheduler* __io_scheduler = nullptr;

void set_io_scheduler(io_scheduler* s)
{
    __io_scheduler = s;
}

io_scheduler* get_io_scheduler()
{
    return __io_scheduler;
}

io_scheduler& get_io_scheduler_check()
{
    assert(__io_scheduler);
    return *__io_scheduler;
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
