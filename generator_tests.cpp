// Copyright (c) 2013 Maciej Gajewski

#include "generator_tests.hpp"
#include "generator.hpp"

#include <iostream>
#include <iomanip>

namespace coroutines { namespace tests {

template<typename NumericType> // now we can choose in which flavour do we want our fibonacci numbers
void fibonacci(const typename corountines::generator<NumericType>::yield_function_type& yield)
{
    NumericType last = 1;
    NumericType current = 1;
    for(;;)
    {
        yield(current);
        NumericType nxt = last + current;
        last = current;
        current = nxt;
    }
}

void fibonacci()
{
    const int N = 10;
    std::cout << "Two fibonacci sequences generated in parallel::" << std::endl;
    std::cout << std::setprecision(3) << std::fixed; // to make floating point number distinguishable
    corountines::generator<int> generator1(fibonacci<int>);
    std::cout << "seq #1: " << generator1.next() << std::endl;
    std::cout << "seq #1: " << generator1.next() << std::endl;

    corountines::generator<double> generator2(fibonacci<double>);
    for(int i = 0; i < N; i++)
    {
        std::cout << "seq #1: " << generator1.next() << std::endl;
        std::cout << "seq #2:       "  << generator2.next() << std::endl;
    }
}

}}


