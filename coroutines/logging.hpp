// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_LOGGING_HPP
#define COROUTINES_LOGGING_HPP

#include <sstream>
#include <iostream>
#include <thread>

namespace coroutines_logging {


inline void _coro_log_impl(std::stringstream& ss) {}

template<typename Arg, typename... Args>
void _coro_log_impl(std::stringstream& ss, const Arg& arg, const Args&... args)
{
    ss << arg;
    _coro_log_impl(ss, args...);
}

template<typename... Args>
void _coro_log(const Args&... args)
{
    std::stringstream ss;
    ss << "[" << std::this_thread::get_id() << "] ";
    _coro_log_impl(ss, args...);
    std::cout << ss.str() << std::endl;
}

}

#ifdef CORO_LOGGING
template<typename... Args> void CORO_LOG (const Args&... args) { coroutines_logging::_coro_log(args...); }
#else
#define CORO_LOG(...) ;
#endif



#endif
