#ifndef COROUTINES_COROUTINE_HPP
#define COROUTINES_COROUTINE_HPP

#include <boost/context/all.hpp>

#include <functional>
#include <string>

namespace coroutines {

class coroutine
{
public:
    typedef std::function<void()> function_type;

    coroutine() = default;
    coroutine(std::string name, function_type& fun);
    ~coroutine();

    coroutine(const coroutine&) = delete;
    coroutine(coroutine&&);
    coroutine& operator=(coroutine&& o) { swap(o); return *this; }

    void swap(coroutine& o);

    void run();

    // can only be called from running coroutine
    static void yield_current();

private:

    void yield();

    static void static_context_function(intptr_t param);
    void context_function();

    std::string _name;
    std::function<void()> _function;

    boost::context::fcontext_t _caller_context;
    boost::context::fcontext_t* _new_context = nullptr;
    char* _stack = nullptr;

};

template<typename Callable>
coroutine make_coroutine(std::string name, Callable&& c)
{
    return coroutine(std::move(name), std::move(c));
}


}

#endif
