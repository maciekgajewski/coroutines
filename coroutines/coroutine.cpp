#include "coroutine.hpp"
#include "channel.hpp"

#include <utility>
#include <cassert>
#include <iostream>

namespace coroutines {

static const unsigned DEFAULT_STACK_SIZE = 64*1024; // 64kb should be enough for anyone :)
static thread_local coroutine* __current_coroutine = nullptr;

coroutine::coroutine(std::string name, function_type&& fun)
    : _name(std::move(name))
    , _function(std::move(fun))
    , _stack(new char[DEFAULT_STACK_SIZE])
{
    _new_context = boost::context::make_fcontext(
                _stack + DEFAULT_STACK_SIZE,
                DEFAULT_STACK_SIZE,
                &coroutine::static_context_function);

}

coroutine::~coroutine()
{
    delete[] _stack;
    if (!_name.empty())
        std::cout << "CORO: '" << _name << "' destroyed" << std::endl;
}

coroutine::coroutine(coroutine&& o)
{
    swap(o);
}

void coroutine::swap(coroutine& o)
{
    std::swap(_name, o._name);
    std::swap(_function, o._function);
    std::swap(_stack, o._stack);
    std::swap(_new_context, o._new_context);
    std::swap(_caller_context, o._caller_context);
}

void coroutine::run()
{
    std::cout << "CORO starting or resuming '" << _name << "'" << std::endl;
    assert(_new_context);

    coroutine* previous = __current_coroutine;
    __current_coroutine = this;

    boost::context::jump_fcontext(&_caller_context, _new_context, reinterpret_cast<intptr_t>(this));

    __current_coroutine = previous;

    std::cout << "CORO '" << _name << "' finished or preemepted" << std::endl;

    if (_epilogue)
    {
        _epilogue();
    }
}

coroutine* coroutine::current_corutine()
{
    return __current_coroutine;
}

void coroutine::yield(epilogue_type epilogue)
{
    assert(__current_coroutine == this);
    _epilogue = std::move(epilogue);
    boost::context::jump_fcontext(_new_context, &_caller_context, 0);
}

void coroutine::static_context_function(intptr_t param)
{
    coroutine* _this = reinterpret_cast<coroutine*>(param);
    _this->context_function();
}

void coroutine::context_function()
{
    std::cout << "CORO: starting '" << _name << "'" << std::endl;
    try
    {
        _function();
    }
    catch(const channel_closed&)
    {
    }
    catch(...)
    {
        std::cerr << "Uncaught exception in " << _name << std::endl;
        std::terminate();
    }

    std::cout << "CORO: finished cleanly '" << _name << "'" << std::endl;

    auto temp = _new_context;
    _new_context = nullptr;// to mark the completion
    boost::context::jump_fcontext(temp, &_caller_context, 0);
}



}
