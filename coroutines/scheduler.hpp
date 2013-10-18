// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_COROUTINE_SCHEDULER_HPP
#define COROUTINES_COROUTINE_SCHEDULER_HPP

#include "coroutines/channel.hpp"
#include "coroutines/coroutine.hpp"
#include "coroutines/processor.hpp"
#include "coroutines/locking_channel.hpp"
#include "coroutines/condition_variable.hpp"
#include "coroutines/processor_container.hpp"

#include <thread>
#include <mutex>
#include <random>

namespace coroutines {

class scheduler
{
public:
    // creates and sets no of max coroutines runnig in parallel
    scheduler(unsigned active_processors = std::thread::hardware_concurrency());

    scheduler(const scheduler&) = delete;

    ~scheduler();

    // launches corountine
    template<typename Callable, typename... Args>
    void go(Callable&& fn, Args&&... args);

    // debug version, with coroutine's name
    template<typename Callable, typename... Args>
    void go(std::string name, Callable&& fn, Args&&... args);

    // debug version, with coroutine's name
    template<typename Callable, typename... Args>
    void go(const char* name, Callable&& fn, Args&&... args);

    // create channel
    template<typename T>
    channel_pair<T> make_channel(std::size_t capacity, const std::string& name)
    {
        return locking_channel<T>::make(*this, capacity, name);
    }

    // wrties current status to stderr
    void debug_dump();

    // wait for all coroutines to complete
    void wait();

    // coroutine interface
    void coroutine_finished(coroutine* coro);

    ///////
    // processor's interface

    void processor_idle(processor* pr);
    void processor_blocked(processor_weak_ptr pr, std::vector<coroutine_weak_ptr>& queue);

    void processor_unblocked(processor_weak_ptr pr);

    void schedule(coroutine_weak_ptr coro);

    template<typename InputIterator>
    void schedule(InputIterator first,  InputIterator last);


private:

    void go(coroutine_ptr&& coro);
    void remove_inactive_processors();

    unsigned random_index();

    const unsigned _active_processors;
    unsigned _blocked_processors = 0;

    processor_container _processors;
    mutex _processors_mutex;

    std::vector<coroutine_ptr> _coroutines;
    mutex _coroutines_mutex;
    std::condition_variable_any _coro_cv;
    std::size_t _max_active_coroutines = 0; // stat counter

    std::vector<coroutine_weak_ptr> _global_queue;

    std::minstd_rand _random_generator;
};


template<typename Callable, typename... Args>
void scheduler::go(Callable&& fn, Args&&... args)
{
    this->go(std::string(), std::forward<Callable>(fn), std::forward<Args>(args)...);
}

template<typename Callable, typename... Args>
void scheduler::go(std::string name, Callable&& fn, Args&&... args)
{
    this->go(make_coroutine(*this, std::move(name), std::bind(std::forward<Callable>(fn), std::forward<Args>(args)...)));
}

template<typename Callable, typename... Args>
void scheduler::go(const char* name, Callable&& fn, Args&&... args)
{

    this->go(make_coroutine(*this, std::string(name), std::bind(std::forward<Callable>(fn), std::forward<Args>(args)...)));
}

} // namespace coroutines

#endif // COROUTINES_COROUTINE_SCHEDULER_HPP
