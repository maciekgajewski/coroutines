// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
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

    // adds work to the queue. Returns false if not successful, because the processor is shutting down
    bool enqueue(coroutine_weak_ptr coro);
    bool enqueue(std::vector<coroutine_weak_ptr>& coros);

    // shuts the processor down, returns true if no tasks in the queue and processor can be destroyed
    bool stop();

    // steals half of work
    void steal(std::vector<coroutine_weak_ptr>& out);

    // number of tasks in the queue (including currently executed)
    unsigned queue_size();

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

    bool _stopped = false;
    std::condition_variable_any _cv;

    std::thread _thread;
};

typedef std::unique_ptr<processor> processor_ptr;
typedef processor* processor_weak_ptr;

} // namespace coroutines

#endif // COROUTINES_PROCESSOR_HPP
