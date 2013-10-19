// Copyright (c) 2013 Maciej Gajewski
#include "coroutines/profiling.hpp"
#include "coroutines/mutex.hpp"

#include <iostream>
#include <forward_list>
#include <array>
#include <cstring>
#include <thread>
#include <fstream>

namespace coroutines_profiling {

static const unsigned DATA_SIZE = 32;

struct record
{
    std::thread::id thread_id;
    std::uint64_t time;
    const char* object_type;
    void* object_id;
    const char* event;
    char data[DATA_SIZE];

    void write(std::ostream& stream);
};

void record::write(std::ostream& stream)
{
    stream << thread_id << "," << time << "," << object_type << "," << object_id << "," << event << "," << data << "\n";
}

static unsigned BLOCK_SIZE = 1000;
static const char* PROFILING_FILE_NAME = "profiling_data.csv";
static thread_local std::thread::id __thread_id;

struct per_thread_data
{
    std::forward_list<record*> blocks;
    unsigned counter = 0;
};

class global_profiling_data
{
public:
    typedef std::forward_list<record*> block_list;
    typedef std::forward_list<per_thread_data> block_list_list;

    global_profiling_data()
    {
    }

    per_thread_data* get_per_thread_data()
    {
        std::lock_guard<coroutines::mutex> lock(_block_list_mutex);
        _block_lists.emplace_front();
        return &_block_lists.front();
    }

    void dump();

    ~global_profiling_data()
    {
        dump();
    }

private:

    block_list_list _block_lists;
    coroutines::mutex _block_list_mutex;
};

static global_profiling_data __global_profiling_data;

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

static thread_local profiling_data __profiling_data;

void profiling_event(const char* object_type, void* object_id, const char* event, const char* data)
{
    record* r = __profiling_data.get_next();

    r->thread_id = __thread_id;
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
    // TODO
    std::cerr << "dumping profiling data to... " << PROFILING_FILE_NAME << std::endl;

    std::ofstream file(PROFILING_FILE_NAME, std::ios_base::out | std::ios_base::trunc);

    for(per_thread_data& thread_data : _block_lists)
    {
        std::cerr << "thread data visited..." << std::endl;
        auto it = thread_data.blocks.begin();
        while(it != thread_data.blocks.end())
        {
            record* block = *it;
            it++;
            unsigned records = BLOCK_SIZE;
            if (it == thread_data.blocks.end())
                records = thread_data.counter;

            std::cerr << "data block of " << records << " records dumped..." << std::endl;

            // write records
            for(unsigned i = 0; i < records; i++)
            {
                block[i].write(file);
            }
        }
    }

    file.close();
    std::cerr << "dumping profiling data completed." << std::endl;
}

}

