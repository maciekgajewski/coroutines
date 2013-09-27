// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_COROUTINE_HPP
#define COROUTINES_COROUTINE_HPP

#include <boost/context/all.hpp>

#include <functional>
#include <string>
#include <memory>

namespace coroutines {

class coroutine
{
public:
    typedef std::function<void()> function_type;
    typedef std::function<void(std::unique_ptr<coroutine>&)> epilogue_type;

    coroutine() = default;
    coroutine(std::string name, function_type&& fun);
    ~coroutine();

    coroutine(const coroutine&) = delete;
    coroutine(coroutine&&) = delete;

    // 'me' can be used transfer corotuine ownership in epilogue
    // I don't like this solution too much, but it seems to be much better than
    // shared_ptr + enable_shared_from_this
    void run(std::unique_ptr<coroutine>& me);

    // returns currently runnig coroutine
    static coroutine* current_corutine();

    void yield(epilogue_type epilogue = epilogue_type());

    std::string name() const { return _name; }

private:


    static void static_context_function(intptr_t param);
    void context_function();

    std::string _name;
    std::function<void()> _function;

    boost::context::fcontext_t _caller_context;
    boost::context::fcontext_t* _new_context = nullptr;
    char* _stack = nullptr;
    epilogue_type _epilogue;

};

template <typename Callable>
class callable_wrapper
{
public:
    callable_wrapper(Callable&& c)
    : _callable(std::move(c))
    {
    }

    void operator()()
    {
        _callable();
    }

private:

    Callable _callable;
};

typedef std::unique_ptr<coroutine> coroutine_ptr;

template<typename Callable>
coroutine_ptr make_coroutine(std::string name, Callable&& c)
{
    callable_wrapper<Callable>* wrapper = new callable_wrapper<Callable>(std::move(c));

    return coroutine_ptr(new coroutine(std::move(name), [wrapper]()
    {
        try
        {
            (*wrapper)();
            delete wrapper;
        }
        catch(...)
        {
            delete wrapper;
            throw;
        }

    }));
}


}

#endif
