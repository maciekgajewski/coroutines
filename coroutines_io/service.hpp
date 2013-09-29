// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_IO_SERVICE_HPP
#define COROUTINES_IO_SERVICE_HPP

#include <boost/asio/io_service.hpp>

namespace coroutines_io {

class service
{
public:

    service();
    ~service();

    boost::asio::io_service& get_io_service() { return _io_service; }

private:

    boost::asio::io_service _io_service;
};

}

#endif
