// Copyright (c) 2013 Maciej Gajewski
#include "coroutines_io/file.hpp"

#include "coroutines_io/globals.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <cassert>

namespace coroutines {

file::file(io_scheduler& srv)
    : base_pollable(srv)
{

}

file::file()
    : base_pollable(get_io_scheduler_check())
{
}

void file::open_for_reading(const std::string& path)
{
    open(path, O_NONBLOCK|O_RDONLY);
}

void file::open_for_writing(const std::string& path)
{
    open(path, O_NONBLOCK|O_WRONLY|O_CREAT|O_TRUNC);
}

void file::open(const std::string& path, int flags)
{
    assert(!is_open());

    int fd = ::open(path.c_str(), flags, 00666);
    if (fd < 0 )
    {
        throw_errno("open");
    }
    else
    {
        set_fd(fd);
    }
}

}
