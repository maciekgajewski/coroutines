#include "coroutine.hpp"

#include <utility>

namespace coroutines {

coroutine::coroutine(coroutine&& o)
{
    swap(o);
}

void coroutine::swap(coroutine& o)
{
    std::swap(_name, o._name);
    std::swap(_function, o._function);
}



}
