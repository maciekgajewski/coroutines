// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "coroutines/processor_container.hpp"

#include <limits>

namespace coroutines {

processor_container::processor_container()
{
}

unsigned processor_container::least_busy_index(unsigned min, unsigned max) const
{
    unsigned min_index = min;
    unsigned min_tasks = std::numeric_limits<unsigned>::max();
    for(unsigned i = min; i < max; i++)
    {
        unsigned qs = _container[i]->queue_size();
        if (qs < min_tasks)
        {
            min_index = i;
            min_tasks = qs;
        }
    }

    return min_index;
}

unsigned processor_container::most_busy_index(unsigned min, unsigned max) const
{
    unsigned max_index = min;
    unsigned max_tasks = 0;
    for(unsigned i = min; i < max; i++)
    {
        unsigned qs = _container[i]->queue_size();
        if (qs > max_tasks)
        {
            max_index = i;
            max_tasks = qs;
        }
    }

    return max_index;
}

void processor_container::stop_all()
{
    for(auto& p : _container)
    {
        p->stop();
    }
}

}
