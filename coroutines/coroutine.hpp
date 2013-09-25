#ifndef COROUTINES_COROUTINE_HPP
#define COROUTINES_COROUTINE_HPP

#include <functional>
#include <string>

namespace coroutines {

class coroutine
{
public:
    typedef std::function<void()> function_type;

    coroutine(std::string name, function_type& fun)
    : _name(std::move(name))
    , _function(std::move(fun))
    { }

    coroutine(const coroutine&) = delete;
    coroutine(coroutine&&);

    void swap(coroutine& o);

private:

    std::string _name;
    std::function<void()> _function;
};

template<typename Callable>
coroutine make_coroutine(std::string name, Callable&& c)
{
    return coroutine(std::move(name), std::move(c));
}


}

#endif
