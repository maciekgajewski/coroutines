// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_ALGORITHM_HPP
#define COROUTINES_ALGORITHM_HPP

#include <algorithm>

namespace coroutines {

// finds a pointer in smart-pointer container
template<typename Container, typename T>
typename Container::iterator find_ptr(Container& ctr, const T* ptr)
{
    auto it = std::find_if(
        ctr.begin(), ctr.end(),
        [ptr](const typename Container::value_type& i)
        {
            return i.get() == ptr;
        });
    return it;
}


}

#endif
