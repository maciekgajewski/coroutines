// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_NAIVE_SCHEDULER_HPP
#define COROUTINES_NAIVE_SCHEDULER_HPP

#include "threaded_channel.hpp"

#include <thread>
#include <functional>
#include <utility>
#include <list>

#include <pthread.h>

namespace coroutines {
// Naive scheduler simply starts each corountine as separate thread
class threaded_scheduler
{
public:

    threaded_scheduler();
    threaded_scheduler(const threaded_scheduler&) = delete;
    ~threaded_scheduler();

    // launches corountine
    template<typename Callable, typename... Args>
    void go(Callable&& fn, Args&&... args);

    // debug version, with coroutine's name
    template<typename Callable, typename... Args>
    void go(std::string name, Callable&& fn, Args&&... args);

    // create channel
    template<typename T>
    channel_pair<T> make_channel(std::size_t capacity)
    {
        return threaded_channel<T, std::condition_variable_any>::make(capacity);
    }

    // wait for all coroutines to complete
    void wait();

private:

    std::list<std::thread> _threads;
    std::mutex _threads_mutex;
};

// functor with thread routine wrapping user-provided coroutine.
// This can be replaced by lambda in C++14
namespace detail {

template<typename Callable>
struct coroutine_wrapper
{
    coroutine_wrapper(std::string name, Callable&& c)
    : _name(std::move(name)), _coroutine(std::move(c))
    { }

    void operator()()
    {
        // name thread
        ::pthread_setname_np(::pthread_self(), _name.substr(0, 15).c_str());

        try
        {
            _coroutine();
            std::cout << "Coroutine " << _name << " exit cleanly" << std::endl;
        }
        catch(const channel_closed& )
        {
            std::cout << "Coroutine " << _name << " exit due to a closed channel" << std::endl;
        }
        catch(...)
        {
            std::cout << "Unhandled exception in coroutine " << _name << ", terminating" << std::endl;
            std::terminate();
        }
    }

    std::string _name;
    Callable _coroutine;
};

template<typename Callable>
coroutine_wrapper<Callable> make_coroutine_wrapper(std::string name, Callable&& c)
{
    return coroutine_wrapper<Callable>(std::move(name), std::move(c));
}

}

template<typename Callable, typename... Args>
void threaded_scheduler::go(Callable&& fn, Args&&... args)
{
    go(std::string(), std::forward<Callable>(fn), std::forward<Args>(args)...);
}

template<typename Callable, typename... Args>
void threaded_scheduler::go(std::string name, Callable&& fn, Args&&... args)
{
    std::lock_guard<std::mutex> lock(_threads_mutex);

    //auto functor = std::bind(std::forward<Callable>(fn), std::forward<Args>(args)...);
    auto functor = detail::make_coroutine_wrapper(std::move(name), std::bind(std::forward<Callable>(fn), std::forward<Args>(args)...));
    std::thread t(std::move(functor));

    _threads.push_back(std::move(t));
}

}

#endif
