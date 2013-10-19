// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_PROFILING_HPP
#define COROUTINES_PROFILING_HPP

#include <thread>
#include <cstdint>

namespace coroutines_profiling {


void profiling_event(const char* object_type, void* object_id, const char* event, const char* data = nullptr);
void dump();

}

#ifdef COROUTINES_PROFILING
#define CORO_PROF coroutines_profiling::profiling_event
#define CORO_PROF_DUMP coroutines_profiling::dump
#else
#define CORO_PROF(...);
#define CORO_PROF_DUMP();
#endif


#endif // PROFILING_HPP
