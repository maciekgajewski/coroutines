// Copyright (c) 2013 Maciej Gajewski

#ifndef PROFILING_READER_HPP
#define PROFILING_READER_HPP

#include <fstream>
#include <thread>
#include <cstdint>
#include <string>
#include <map>

namespace profiling_reader {

struct record_type
{
    std::int64_t time;
    std::size_t thread_id;
    std::string object_type;
    std::uintptr_t object_id;
    std::string event;
    std::string data;
};

class reader
{
public:
    reader(const std::string& file_name);

    double ticks_per_ns() const { return _ticks_per_ns; }

    // visits all records in chronological order
    template<typename Callable>
    void for_each_by_time(Callable c) const
    {
        for(auto& p : _by_time)
        {
            c(p.second);
        }
    }

private:

    std::multimap<std::int64_t, record_type> _by_time;
    double _ticks_per_ns = 0.0;
};

} // namespace profiling

#endif // PROFILING_READER_HPP
