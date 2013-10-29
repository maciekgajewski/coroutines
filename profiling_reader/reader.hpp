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
    std::int64_t time_ns;
    std::int64_t ticks;
    std::size_t thread_id;
    std::string object_type;
    std::uintptr_t object_id;
    std::uint32_t ordinal;
    std::string event;
    std::string data;
};

class reader
{
public:
    reader(const std::string& file_name);

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

    // index by time. key is time_ns
    std::multimap<std::int64_t, record_type> _by_time;
};

} // namespace profiling

#endif // PROFILING_READER_HPP
