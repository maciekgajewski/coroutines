#ifndef COROUTINES_PROCESSOR_HPP
#define COROUTINES_PROCESSOR_HPP

#include "coroutines/coroutine.hpp"
#include "coroutines/mutex.hpp"

#include <deque>
#include <vector>
#include <thread>
#include <memory>

namespace coroutines {

class scheduler;

class processor
{
public:
    processor(scheduler& sched);
    processor(const processor&) = delete;
    ~processor();

    // adds work to the queue
    void enqueue(coroutine_weak_ptr coro);
    void enqueue(std::vector<coroutine_weak_ptr>& coros);

    // steals half of work
    void steal(std::vector<coroutine_weak_ptr>& out);

    // block/unblock
    void block();
    void unblock();

    static processor* current_processor();

private:

    void routine();
    void wakeup();
    void stop_and_join();

    scheduler& _scheduler;

    std::deque<coroutine_weak_ptr> _queue;
    mutex _queue_mutex;

    bool _running = false; // is currently running or waiting?
    bool _stopped = false;
    mutex _runnng_mutex;
    std::condition_variable_any _running_cv;

    std::thread _thread;
};

typedef std::unique_ptr<processor> processor_ptr;
typedef processor* processor_weak_ptr;

} // namespace coroutines

#endif // COROUTINES_PROCESSOR_HPP
