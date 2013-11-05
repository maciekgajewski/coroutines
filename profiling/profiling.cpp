// Copyright (c) 2013 Maciej Gajewski
#include "profiling/profiling.hpp"

#include <iostream>
#include <forward_list>
#include <list>
#include <cstring>
#include <thread>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <mutex>
#include <cassert>

#include <time.h>
#include <unistd.h>
#include <pthread.h>

namespace profiling {

static const unsigned DATA_SIZE = 64;

static std::uint64_t __sys_tv_sec_base = 0;
static std::uint64_t __clock_base; // first-ever tcs call
static double __ticks_per_ns = 0.0;

struct record
{
    std::int64_t time;
    std::thread::id thread_id;
    const char* object_type;
    void* object_id;
    std::uint32_t ordinal = 0;
    const char* event;
    char data[DATA_SIZE];

    void write(std::ostream& stream, std::uint64_t time_ns);
};

void record::write(std::ostream& stream, std::uint64_t time_ns)
{
    stream << time_ns << "," << time << "," << std::hash<std::thread::id>()(thread_id) << "," << object_type << "," << (std::uintptr_t)object_id << "," << ordinal << "," << event << "," << data << "\n";
}

static unsigned BLOCK_SIZE = 1000;
static const char* PROFILING_FILE_NAME = "profiling_data.csv";

struct per_thread_data
{
    std::forward_list<record*> blocks;
    unsigned counter = 0;
    std::thread::id thread_id;
    // tsc<->sys time relation. time from both clocks taken at the same time
    std::uint64_t sys_ns;
    std::uint64_t tsc_ticks;
};

// returns tsc, in ticks. Takes 30-40 ticks
inline std::uint64_t get_tsc()
{
    std::uint32_t low, high;
    asm volatile (
        "rdtsc"
        : "=a" (low), "=d" (high));
    return std::uint64_t(high) << 32 | low;
}


// returns system time, in ns (from some  arbitrary point). Takes 2us
static std::int64_t get_systime()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (std::int64_t(ts.tv_sec) - __sys_tv_sec_base)*1000000000 + ts.tv_nsec;
}

// returns ticks per nanosecond
static double calibrate_clock();

class global_profiling_data
{
public:
    typedef std::forward_list<record*> block_list; // needs to be fast and simple
    typedef std::list<per_thread_data> block_list_list; // modified once per thread, we can have some sophistication

    global_profiling_data()
    {
        __clock_base = get_tsc();
        __ticks_per_ns = calibrate_clock();

        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        __sys_tv_sec_base = ts.tv_sec;
    }

    // caled when thred instrumentation is created, in the thread
    per_thread_data* get_per_thread_data()
    {
        std::lock_guard<std::mutex> lock(_block_list_mutex);
        _block_lists.emplace_front();

        // stick this thread to a core
        stick_to_core(_core++);

        return &_block_lists.front();
    }

    void dump();

    ~global_profiling_data()
    {
        dump();
    }

    std::int64_t sys_tv_sec_base;

private:

    void stick_to_core(int core)
    {
        int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
        int core_adj = core % num_cores;

        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_adj, &cpuset);

        pthread_t current_thread = pthread_self();
        pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
    }


    block_list_list _block_lists;
    std::mutex _block_list_mutex;
    int _core = 0;
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
        _data->thread_id = __thread_id;

        record* block = new record[BLOCK_SIZE];
        _data->blocks.push_front(block);

        // tsc <-> sys relation in this thread
        std::int64_t sys1 = get_systime();
        _data->tsc_ticks = get_tsc() - __clock_base;
        std::int64_t sys2 = get_systime();
        _data->sys_ns = (sys1+sys2)/2;
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
            block[0].time = get_tsc() - __clock_base;
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

void profiling_event(const char* object_type, void* object_id, const char* event, std::uint32_t ordinal, const char* data)
{
    // this is hotpath!

    record* r = __profiling_data.get_next();

    r->thread_id = __thread_id;
    r->time = get_tsc() - __clock_base;
    r->object_type = object_type;
    r->object_id = object_id;
    r->ordinal = ordinal;
    r->event = event;
    if (data)
        std::strncpy(r->data, data, DATA_SIZE-1);
    else
        r->data[0] = 0;
}

void dump()
{
    __global_profiling_data.dump();
}

// tick->ns converter
class tsc_to_ns_functor
{
public:

    // sys_ns and tcs are clock values taken at the same time
    tsc_to_ns_functor(double ticks_per_ns, std::int64_t calib_sys_ns, std::int64_t calib_tcs)
    {
        _ticks_per_ns = ticks_per_ns;
        _ns_shift = calib_sys_ns - calib_tcs/ticks_per_ns;
    }

    // returns time in ns
    std::int64_t operator()(std::int64_t tcs) const
    {
        return tcs/_ticks_per_ns + _ns_shift;
    }

private:
    double _ticks_per_ns;
    std::int64_t _ns_shift;
};

void global_profiling_data::dump()
{
    std::cerr << "dumping profiling data to... " << PROFILING_FILE_NAME << std::endl;
    try
    {
        std::ofstream file(PROFILING_FILE_NAME, std::ios_base::out | std::ios_base::trunc);

        unsigned counter = 0;

        // store the earliest clock calibration event
        std::int64_t min_sys_ns = _block_lists.back().sys_ns;

        for(per_thread_data& thread_data : _block_lists)
        {

            tsc_to_ns_functor tcs_to_ns(__ticks_per_ns, thread_data.sys_ns - min_sys_ns, thread_data.tsc_ticks);

            unsigned records = thread_data.counter; // only valid for the first block
            for(record* block : thread_data.blocks)
            {
                // write records
                for(unsigned i = 0; i < records; i++)
                {
                    block[i].write(file, tcs_to_ns(block[i].time));
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
        std::uint64_t tsc_start = get_tsc();
        auto clock_start = std::chrono::high_resolution_clock::now();

        for(;;)
        {
            for(unsigned j = 0; j < REPETITIONS; j++);
            auto now =  std::chrono::high_resolution_clock::now();
            if ((now - clock_start) > std::chrono::microseconds(1000))
                break;
        }

        std::uint64_t tsc_end = get_tsc();
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

