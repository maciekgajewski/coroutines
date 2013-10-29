// Copyright (c) 2013 Maciej Gajewski

#include "profiling_reader/reader.hpp"

#include <iostream>
#include <string>
#include <unordered_map>

// holds process state
struct processor_state
{
    unsigned tasks_run = 0;
    double routine_started = 0;
    double time_in_coroutines = 0;

    double time_first_coro_start = 0;
    double time_last_coro_start = 0;
    double time_last_coro_end = 0;
};

void analyze(const profiling_reader::reader& reader)
{
    std::unordered_map<std::size_t, processor_state> processors;

    std::cout << "analyzing..." << std::endl;

    reader.for_each_by_time([&processors](const profiling_reader::record_type& record)
    {
        if (record.object_type == "processor" && record.event == "routine started")
        {
            processor_state& ps = processors[record.thread_id];
            ps.routine_started = record.time_ns;
        }

        if (record.object_type == "coroutine")
        {
            processor_state& ps = processors[record.thread_id];

            if (record.event == "enter")
            {
                if (ps.time_first_coro_start == 0)
                    ps.time_first_coro_start = record.time_ns;
                ps.time_last_coro_start = record.time_ns;
                ps.tasks_run++;
            }
            else if (record.event == "exit")
            {
                ps.time_last_coro_end = record.time_ns;
                ps.time_in_coroutines += (record.time_ns - ps.time_last_coro_start);
            }
        }
    });

    // display some info
    for(auto& p : processors)
    {
        const processor_state& ps = p.second;

        if (ps.routine_started == 0)
            continue;

        std::cout << "Process info" << std::endl;
        std::cout << "  *                    thread id: " << p.first << std::endl;
        std::cout << "  *                    tasks run: " << ps.tasks_run << std::endl;
        std::cout << "  *              routine started: " << ps.routine_started << std::endl;
        std::cout << "  *                time in coros: " << ps.time_in_coroutines << " ns" << std::endl;
        std::cout << "  * time from first to last coro: " << (ps.time_last_coro_end - ps.time_first_coro_start) << " ns" << std::endl;
        std::cout << std::endl;
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "USAGE: profiling_analyze PROFILING_FILE" << std::endl;
        return 1;
    }

    try
    {
        profiling_reader::reader reader(argv[1]);

        analyze(reader);

    }
    catch(const std::exception& e)
    {
        std::cerr << "Error : " << e.what() << std::endl;
        return 2;
    }

    return 0;
}

