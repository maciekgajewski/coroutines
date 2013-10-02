// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_FILE_HPP
#define COROUTINES_FILE_HPP

#include "coroutines_io/base_pollable.hpp"

namespace coroutines {

// disk file
class file : public base_pollable
{
public:

    file(service& srv);
    file(); // uses get_service_check()

    void open_for_reading(const std::string& path);
    void open_for_writing(const std::string& path);

private:

    void open(const std::string& path, int flags);
};

}

#endif
