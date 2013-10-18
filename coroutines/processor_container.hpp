// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_PROCESSOR_CONTAINER_HPP
#define COROUTINES_PROCESSOR_CONTAINER_HPP

#include "coroutines/processor.hpp"
#include "coroutines/algorithm.hpp"

#include <vector>

namespace coroutines {

// special purpose container for storing processors.
// the container is divided into two areas - acticve and inactive.
// active is the first max_active processors
class processor_container
{
public:
    processor_container(unsigned max_active);

    unsigned size() const { return _container.size(); }

    unsigned index_of(processor* pc) const
    {
        auto it = find_ptr(_container, pc);
        return std::distance(_container.begin(), it);
    }

    void emplace_back(scheduler& sched) { _container.push_back(processor_ptr(new processor(sched))); }

    // inserts at index
    void insert(unsigned index, scheduler& sched)
    {
        _container.insert(_container.begin() + index, processor_ptr(new processor(sched)));
    }

    // will wait for thread to join, be sure to stop the processor
    void pop_back() { _container.pop_back(); }

    processor& operator[](unsigned i) { return *_container[i]; }
    processor& back() { return *_container.back(); }

    // returns processor with least and most coros in queue

    unsigned least_busy_index(unsigned min, unsigned max) const;
    unsigned most_busy_index(unsigned min, unsigned max) const;

    void swap(unsigned a, unsigned b) { std::swap(_container[a], _container[b]); }

    void stop_all();

private:

    std::vector<processor_ptr> _container;
    unsigned _max_active;
};

}

#endif
