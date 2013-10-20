// Copyright (c) 2013 Maciej Gajewski
#ifndef PROFILING_PROFILING_HPP
#define PROFILING_PROFILING_HPP

namespace profiling {

void profiling_event(const char* object_type, void* object_id, const char* event, const char* data = nullptr);
void dump();

}

#ifdef COROUTINES_PROFILING
#define CORO_PROF profiling::profiling_event
#define CORO_PROF_DUMP profiling::dump
#else
#define CORO_PROF(...);
#define CORO_PROF_DUMP();
#endif


#endif
