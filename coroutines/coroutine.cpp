// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#include "coroutines/coroutine.hpp"
#include "coroutines/channel.hpp"
#include "coroutines/scheduler.hpp"

//#define CORO_LOGGING
#include "coroutines/logging.hpp"

#include "profiling/profiling.hpp"

#include <utility>
#include <cassert>
#include <iostream>

namespace coroutines {

static const unsigned DEFAULT_STACK_SIZE = 64*1024; // 64kb should be enough for anyone :)
static thread_local coroutine* __current_coroutine = nullptr;

coroutine::coroutine(scheduler& parent, std::string name, function_type&& fun)
    : _function(std::move(fun))
    , _stack(new char[DEFAULT_STACK_SIZE])
#ifdef COROUTINES_SPINLOCKS_PROFILING
    , _run_mutex(std::string(name + " run mutex").c_str())
#endif
    , _parent(parent)
    , _name(std::move(name))
{
    CORO_PROF("coroutine", this, "created", _name.c_str());

    _new_context = boost::context::make_fcontext(
                _stack + DEFAULT_STACK_SIZE,
                DEFAULT_STACK_SIZE,
                &coroutine::static_context_function);

}

coroutine::~coroutine()
{
    CORO_PROF("coroutine", this, "destroyed");
    CORO_LOG("CORO=", this, " destroyed");
    if (_new_context)
    {
        std::cerr<< "FATAL: coroutine '" << _name << "' destroyed before completed. Last checkpoint: " << _last_checkpoint << std::endl;
    }
    assert(!_new_context);
    delete[] _stack;
}

void coroutine::run()
{
    {
        std::lock_guard<mutex> lock(_run_mutex); // the coro may be reshdelued in epilogue, and run imemdiately in different thread

        CORO_LOG("CORO starting or resuming '", _name, "'");
        assert(_new_context);

        coroutine* previous = __current_coroutine;
        __current_coroutine = this;

        CORO_PROF("coroutine", this, "enter");
        boost::context::jump_fcontext(&_caller_context, _new_context, reinterpret_cast<intptr_t>(this));
        CORO_PROF("coroutine", this, "exit");

        __current_coroutine = previous;

        CORO_LOG("CORO=", this, " finished or preemepted");

        if (_epilogue)
        {
            assert(_new_context);
            _epilogue(this);
            _epilogue = nullptr;
        }
    }

    if (!_new_context)
    {
        _parent.coroutine_finished(this); // this wil destroy the object (delete this)
    }
}

coroutine* coroutine::current_corutine()
{
    return __current_coroutine;
}

void coroutine::yield(const std::string& checkpoint_name, epilogue_type epilogue)
{
    assert(__current_coroutine == this);

    _last_checkpoint = checkpoint_name;

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
    CORO_LOG("CORO: starting '", _name, "'");
    try
    {
        _last_checkpoint = "started";
        _function();
        _last_checkpoint = "finished cleanly";
    }
    catch(const channel_closed&)
    {
        _last_checkpoint = "finished after channel close";
    }
    catch(const std::exception& e)
    {
        _last_checkpoint = std::string("uncaught exception: ") + e.what();
        std::cerr << "Uncaught exception in " << _name << " : " << e.what() << std::endl;
        std::terminate();
    }
    catch(...)
    {
        _last_checkpoint = "uncaught exception";
        std::cerr << "Uncaught exception in " << _name << std::endl;
        std::terminate();
    }

    CORO_LOG("CORO: finished cleanly '", _name, "'");

    auto temp = _new_context;
    _new_context = nullptr;// to mark the completion
    boost::context::jump_fcontext(temp, &_caller_context, 0);
}



}
