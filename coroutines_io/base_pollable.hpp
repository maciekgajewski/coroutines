// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_BASE_POLLABLE_HPP
#define COROUTINES_BASE_POLLABLE_HPP

#include "coroutines/channel.hpp"

#include <system_error>

namespace coroutines {

class service;

class base_pollable
{
public:
    base_pollable(service& srv);
    base_pollable(const base_pollable&) = delete;
    base_pollable(base_pollable&& o);

    ~base_pollable();

    void close();

    // read all unless how_much or EOF
    std::size_t read(char* buf, std::size_t how_much);

    // reads whatever is available, blocks only if nothing's there
    std::size_t read_some(char* buf, std::size_t how_much);

    // reads until buffer is full, contains pattern or EOF
    std::size_t read_unitl(char* buf, std::size_t how_much,const std::string& pattern);

    // write all
    std::size_t write(const char* buf, std::size_t how_much);


protected:

    void set_fd(int get_fd);

    void wait_for_readable();
    void wait_for_writable();

    int get_fd() const { return _fd; }
    bool is_open() const { return _fd != -1; }

    service& get_service() { return _service; }

private:

    int _fd = -1;

    service& _service;

    channel_reader<std::error_code> _reader;
    channel_writer<std::error_code> _writer;
};

}

#endif
