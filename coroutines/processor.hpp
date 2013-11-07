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

    // adds work to the queue. Returns false if not successful, because the processor is shutting down or blocked
    bool enqueue(coroutine_weak_ptr coro);

    template<typename InputIterator>
    bool enqueue(InputIterator first, InputIterator last);

    // shuts the processor down, returns true if no tasks in the queue and processor can be destroyed
    // if false returned, the processor will stop after exhaustingf the queue
    bool stop();

    // will stop the processor only if it has empty queue and is not doigng anything
    // if false is returned, the processor will continue
    bool stop_if_idle();

    // steals half of work
    void steal(std::vector<coroutine_weak_ptr>& out);

    // number of tasks in the queue (including currently executed)
    unsigned queue_size();

    static processor* current_processor();

private:

    void routine();
    void wakeup();

    scheduler& _scheduler;

    std::deque<coroutine_weak_ptr> _queue;
    mutex _queue_mutex;

    bool _stopped = false;
    bool _blocked = false;
    std::condition_variable_any _cv;
    bool _executing = false;

    std::thread _thread;
};

typedef std::unique_ptr<processor> processor_ptr;
typedef processor* processor_weak_ptr;


} // namespace coroutines

#endif // COROUTINES_PROCESSOR_HPP
