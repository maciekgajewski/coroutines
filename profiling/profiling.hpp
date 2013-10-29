// Copyright (c) 2013 Maciej Gajewski
#ifndef PROFILING_PROFILING_HPP
#define PROFILING_PROFILING_HPP

#include <atomic>
#include <cstdint>

namespace profiling {

void profiling_event(const char* object_type, void* object_id, const char* event, std::uint32_t ordinal, const char* data);
inline void profiling_event(const char* object_type, void* object_id, const char* event, const char* data = nullptr)
{
    profiling_event(object_type, object_id, event, 0, data);
}

void dump();

}

#ifdef COROUTINES_PROFILING
#define CORO_PROF profiling::profiling_event
#define CORO_PROF_DUMP profiling::dump
#define CORO_PROF_DECLARE_COUNTER(name) static std::atomic<std::uint32_t> __coro_prof_counter ## name;
#define CORO_PROF_COUNTER(name) __coro_prof_counter ## name ++;
#else
#define CORO_PROF(...);
#define CORO_PROF_DUMP();
#define CORO_PROF_DECLARE_COUNTER(name);
#define CORO_PROF_COUNTER(name)
#endif


#endif
