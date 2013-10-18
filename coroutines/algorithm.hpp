// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_ALGORITHM_HPP
#define COROUTINES_ALGORITHM_HPP

#include <algorithm>

// various algorithms used in the lib

namespace coroutines {

// finds a pointer in smart-pointer container
template<typename Container, typename T>
auto find_ptr(Container& ctr, const T* ptr) -> decltype(ctr.begin())
{
    auto it = std::find_if(
        ctr.begin(), ctr.end(),
        [ptr](const typename Container::value_type& i)
        {
            return i.get() == ptr;
        });
    return it;
}

// copies elements from IN to OUT, if predicate is true, trnasofrimg tjhem using UNARY_OP
template<typename InputIterator, typename OutputIterator, typename UnaryOp, typename Predicate>
void transform_if(InputIterator begin, InputIterator end, OutputIterator out, UnaryOp trans, Predicate pred)
{
    std::for_each(begin, end, [&](const decltype(*begin)& item)
    {
        if(pred(item))
        {
            *(out++) = trans(item);
        }
    });
}


}

#endif
