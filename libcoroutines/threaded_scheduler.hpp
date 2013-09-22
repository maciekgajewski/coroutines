// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_NAIVE_SCHEDULER_HPP
#define COROUTINES_NAIVE_SCHEDULER_HPP

#include <thread>
#include <functional>
// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include <utility>
#include <list>

#include "threaded_channel.hpp"

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

    // create channel
    template<typename T>
    channel_pair<T> make_channel(std::size_t capacity) { return threaded_channel<T>::make(capacity); }

    // wait for all coroutines to complete
    void wait();

private:

    std::list<std::thread> _threads;
};

// functor with thread routine wrapping user-provided coroutine.
// This can be replaced by lambda in C++14
namespace detail {

template<typename Callable>
struct coroutine_wrapper
{
    coroutine_wrapper(Callable&& c)
    : _coroutine(std::move(c))
    { }

    void operator()()
    {
        try
        {
            _coroutine();
            std::cout << "Coroutine exit cleanly" << std::endl;
        }
        catch(const channel_closed& )
        {
            std::cout << "Coroutine exit due to a closed channel" << std::endl;
        }
        catch(...)
        {
            std::cout << "Unhandled exception in coroutine, terminating" << std::endl;
            std::terminate();
        }
    }

    Callable _coroutine;
};

template<typename Callable>
coroutine_wrapper<Callable> make_coroutine_wrapper(Callable&& c)
{
    return coroutine_wrapper<Callable>(std::move(c));
}

}

template<typename Callable, typename... Args>
void threaded_scheduler::go(Callable&& fn, Args&&... args)
{
    //auto functor = std::bind(std::forward<Callable>(fn), std::forward<Args>(args)...);
    auto functor = detail::make_coroutine_wrapper(std::bind(std::forward<Callable>(fn), std::forward<Args>(args)...));
    std::thread t(std::move(functor));

    _threads.push_back(std::move(t));
}

}

#endif
