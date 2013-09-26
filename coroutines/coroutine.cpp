#include "coroutine.hpp"
#include "channel.hpp"

#include <utility>
#include <cassert>
#include <iostream>

namespace coroutines {

static const unsigned DEFAULT_STACK_SIZE = 64*1024; // 64kb should be enough for anyone :)
static thread_local coroutine* __current_coroutine = nullptr;

coroutine::coroutine(std::string name, function_type& fun)
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
    assert(_new_context);

    coroutine* previous = __current_coroutine;
    __current_coroutine = this;

    boost::context::jump_fcontext(&_caller_context, _new_context, reinterpret_cast<intptr_t>(this));

    __current_coroutine = previous;
}

void coroutine::yield_current()
{
    assert(__current_coroutine);
    __current_coroutine->yield();
}

void coroutine::yield()
{
    assert(__current_coroutine == this);
    boost::context::jump_fcontext(_new_context, &_caller_context, 0);
}

void coroutine::static_context_function(intptr_t param)
{
    coroutine* _this = reinterpret_cast<coroutine*>(param);
    _this->context_function();
}

void coroutine::context_function()
{
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

    _new_context = nullptr; // this mark the completion
}



}
