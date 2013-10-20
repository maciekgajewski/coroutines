// Copyright (c) 2013 Maciej Gajewski
#include "profiling/profiling.hpp"

#include <iostream>
#include <forward_list>
#include <array>
#include <cstring>
#include <thread>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <mutex>

namespace profiling {

static const unsigned DATA_SIZE = 32;

struct record
{
    std::int64_t time;
    std::thread::id thread_id;
    const char* object_type;
    void* object_id;
    const char* event;
    char data[DATA_SIZE];

    void write(std::ostream& stream);
};

void record::write(std::ostream& stream)
{
    stream << time << "," << std::hash<std::thread::id>()(thread_id) << "," << object_type << "," << (std::uintptr_t)object_id << "," << event << "," << data << "\n";
}

static unsigned BLOCK_SIZE = 1000;
static const char* PROFILING_FILE_NAME = "profiling_data.csv";

struct per_thread_data
{
    std::forward_list<record*> blocks;
    unsigned counter = 0;
};

inline std::uint64_t get_rdtsc()
{
    std::uint32_t low, high;
    asm volatile ("rdtsc" : "=a" (low), "=d" (high));
    return std::uint64_t(high) << 32 | low;
}

// returns ticks per nanosecond
static double calibrate_clock();

class global_profiling_data
{
public:
    typedef std::forward_list<record*> block_list;
    typedef std::forward_list<per_thread_data> block_list_list;

    global_profiling_data()
    {
        _ticks_per_ns = calibrate_clock();
        clock_base = get_rdtsc();
    }

    per_thread_data* get_per_thread_data()
    {
        std::lock_guard<std::mutex> lock(_block_list_mutex);
        _block_lists.emplace_front();
        return &_block_lists.front();
    }

    void dump();

    ~global_profiling_data()
    {
        dump();
    }

    std::uint64_t clock_base;

private:

    block_list_list _block_lists;
    std::mutex _block_list_mutex;
    double _ticks_per_ns;
};

static global_profiling_data __global_profiling_data;
thread_local std::thread::id __thread_id;

class profiling_data
{
public:

    profiling_data()
    {
        __thread_id = std::this_thread::get_id();
        _data = __global_profiling_data.get_per_thread_data();

        record* block = new record[BLOCK_SIZE];
        _data->blocks.push_front(block);
    }

    ~profiling_data()
    {
        std::cout << "porfiling data destroyed" << std::endl;
    }

    record* get_next()
    {
        if (_data->counter < BLOCK_SIZE)
        {
            return _data->blocks.front() + _data->counter++;
        }
        else
        {
            // need to allocate another one
            record* block = new record[BLOCK_SIZE];
            block[0].thread_id = __thread_id;
            block[0].time = get_rdtsc();
            block[0].object_type = "profiler";
            block[0].object_id = this;
            block[0].event = "new block";

            _data->blocks.push_front(block);
            _data->counter = 2;
            return block + 1;
        }
    }

private:
    per_thread_data* _data;
};

thread_local profiling_data __profiling_data;

void profiling_event(const char* object_type, void* object_id, const char* event, const char* data)
{
    record* r = __profiling_data.get_next();

    r->thread_id = __thread_id;
    r->time = get_rdtsc() - __global_profiling_data.clock_base;
    r->object_type = object_type;
    r->object_id = object_id;
    r->event = event;
    if (data)
        std::strncpy(r->data, data, DATA_SIZE);
    else
        r->data[0] = 0;
}

void dump()
{
    __global_profiling_data.dump();
}

void global_profiling_data::dump()
{
    std::cerr << "dumping profiling data to... " << PROFILING_FILE_NAME << std::endl;
    try
    {
        std::ofstream file(PROFILING_FILE_NAME, std::ios_base::out | std::ios_base::trunc);

        // dump calibration data
        std::stringstream ss;
        ss << _ticks_per_ns;
        profiling_event("global_profiling_data", this, "clock calibration", ss.str().c_str());

        unsigned counter = 0;

        for(per_thread_data& thread_data : _block_lists)
        {
            unsigned records = thread_data.counter; // only valid for the first block
            for(record* block : thread_data.blocks)
            {
                // write records
                for(unsigned i = 0; i < records; i++)
                {
                    block[i].write(file);
                    counter ++;
                }
                // all subsequent blocks are fully utilized
                records = BLOCK_SIZE;
            }
        }

        file.close();

        std::cerr << "dumping profiling data completed, " << counter << " records written" << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error dumping profiling data: " << e.what() << std::endl;
    }
}


double calibrate_clock()
{
    static const unsigned CYCLES = 11;
    static const unsigned REPETITIONS = 1000000;

    std::vector<double> mplyers;
    mplyers.reserve(CYCLES);


    for(unsigned i = 0; i < CYCLES; i++)
    {
        std::uint64_t tsc_start = get_rdtsc();
        auto clock_start = std::chrono::high_resolution_clock::now();

        for(;;)
        {
            for(unsigned j = 0; j < REPETITIONS; j++);
            auto now =  std::chrono::high_resolution_clock::now();
            if ((now - clock_start) > std::chrono::microseconds(1000))
                break;
        }

        std::uint64_t tsc_end = get_rdtsc();
        auto clock_end =  std::chrono::high_resolution_clock::now();

        std::uint64_t tsc_diff = tsc_end - tsc_start;
        std::uint64_t nano_diff = (clock_end-clock_start)/std::chrono::nanoseconds(1);
        double tick_per_nano = double(tsc_diff) / nano_diff;

        mplyers.push_back(tick_per_nano);
    }

    // return median
    std::sort(mplyers.begin(), mplyers.end());
    return mplyers[std::ceil(CYCLES/2.0)];
}




}

