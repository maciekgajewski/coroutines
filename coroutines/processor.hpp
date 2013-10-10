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

    // adds work to the queue
    void enqeue(coroutine_weak_ptr coro);
    void enqeue(std::vector<coroutine_weak_ptr>& coros);

    // steals half of work
    void steal(std::vector<coroutine_weak_ptr>& out);

    // shutdown
    void stop_and_join();

    // block/unblock
    void block();
    void unblock();

private:

    void routine();
    void wakeup();

    scheduler& _scheduler;

    std::deque<coroutine_weak_ptr> _queue;
    mutex _queue_mutex;

    bool _running = false; // is currently running or waiting?
    bool _stopped = false;
    bool _blocked = false;
    mutex _runnng_mutex;
    std::condition_variable_any _running_cv;

    std::thread _thread;
};

typedef std::unique_ptr<processor> processor_ptr;
typedef processor* processor_weak_ptr;

} // namespace coroutines

#endif // COROUTINES_PROCESSOR_HPP
