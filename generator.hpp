// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_GENERATOR_HPP
#define COROUTINES_GENERATOR_HPP

#include <stdexcept>
#include <exception>
#include <iostream>
#include <utility>
#include <functional>

#include <boost/context/all.hpp>
#include <boost/optional.hpp>

namespace corountines {

// exceptioin used to signalizethat the generator is stopped
struct generator_stopped : public std::exception
{
public:

    virtual const char* what() const noexcept { return "generator stopped"; }
};

// Python-style generator
template<typename ReturnType>
class generator
{
public:

    typedef std::function<void(std::function<void (ReturnType)>)> CallableType;
    typedef ReturnType return_type;

    generator(const CallableType& c, std::size_t stack_size);
    ~generator();

    bool stopped() const noexcept { return !_stack; }

    // executes callable until the next yeld, or throw generator_stopped exception
    ReturnType get();

private:

    static void context_function(intptr_t);
    void yield(ReturnType val);

    CallableType _callable;
    char* _stack;

    boost::optional<ReturnType> _return_value;
    std::exception_ptr _exception;


    boost::context::fcontext_t* _own_context;
    boost::context::fcontext_t _caller_context;
};

template<typename ReturnType, typename CallableType>
generator<ReturnType> make_generator(const CallableType& c, std::size_t stack_size = 64*1024)
{
    return generator<ReturnType>(c, stack_size);
}

template<typename ReturnType>
generator<ReturnType>::generator(const CallableType& c, std::size_t stack_size)
    : _callable(c), _stack(nullptr)
{
    // allocate stack
    _stack = new char[stack_size];

    // create context
    char* sp = _stack + stack_size; // TODO this is architecture specific. How to make it universal?
    _own_context = boost::context::make_fcontext(sp, stack_size, &generator<ReturnType>::context_function);
}

template<typename ReturnType>
generator<ReturnType>::~generator()
{
    delete _stack;
}

template<typename ReturnType>
ReturnType generator<ReturnType>::get()
{
    if (!_stack)
    {
        throw generator_stopped();
    }

    // clear return variables
    _return_value.reset();
    _exception = nullptr;

    // jump into the context
    boost::context::jump_fcontext(&_caller_context, _own_context, intptr_t(this));

    if (_exception)
    {
        delete _stack;
        _stack = nullptr;
        std::rethrow_exception(_exception);
    }
    else if (_return_value)
    {
        return *_return_value;
    }
    else
    {
        delete _stack;
        _stack = nullptr;
        throw generator_stopped();
    }

}

template<typename ReturnType>
void generator<ReturnType>::yield(ReturnType val)
{
    // called by the function
    _return_value = val;
    boost::context::jump_fcontext(_own_context, &_caller_context, 0);
}

template<typename ReturnType>
void generator<ReturnType>::context_function(intptr_t _this_intptr)
{
    auto _this = (generator<ReturnType>*)(_this_intptr);

    try
    {
        _this->_callable([_this](ReturnType val) { _this->yield(val); });
    }
    catch(...)
    {
        _this->_exception = std::current_exception();
    }

    // jump back
    boost::context::jump_fcontext(_this->_own_context, &_this->_caller_context, 0);
}



// usage:
// generator<int> g = make_generator<int>([](yield)
// {
//      ...
//      yield(7);
//      ...
//      yield(42);
// }

};

#endif
