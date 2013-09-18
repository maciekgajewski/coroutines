// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_GENERATOR_HPP
#define COROUTINES_GENERATOR_HPP

#include <stdexcept>
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

    bool stopped() const noexcept { return _stopped; }

    // executes callable until the next yeld, or throw generator_stopped exception
    ReturnType get();

private:

    static void context_function(intptr_t);
    void yield(ReturnType val);

    CallableType _callable;
    char* _stack;
    boost::optional<ReturnType> _return_value;
    boost::context::fcontext_t* _own_context;
    boost::context::fcontext_t _caller_context;
    bool _stopped;
};

template<typename ReturnType, typename CallableType>
generator<ReturnType> make_generator(const CallableType& c, std::size_t stack_size = 64*1024)
{
    return generator<ReturnType>(c, stack_size);
}

template<typename ReturnType>
generator<ReturnType>::generator(const CallableType& c, std::size_t stack_size)
    : _callable(c), _stack(nullptr), _stopped(false)
{
    // allocate stack
    _stack = new char[stack_size];

    // create context
    _own_context = boost::context::make_fcontext(_stack, stack_size, &generator<ReturnType>::context_function);
    //boost::context::make_fcontext(_stack, stack_size, f1);
}

template<typename ReturnType>
generator<ReturnType>::~generator()
{
    delete _stack;
}

template<typename ReturnType>
ReturnType generator<ReturnType>::get()
{
    if (_stopped)
    {
        throw generator_stopped();
    }

    // jump into the context
    std::cout << "GG jumping into context" << std::endl;
    boost::context::jump_fcontext(&_caller_context, _own_context, intptr_t(this));
    std::cout << "GG returned back" << std::endl;

    if (_return_value)
    {
        return *_return_value;
    }
    else
    {
        throw generator_stopped();
    }

}

template<typename ReturnType>
void generator<ReturnType>::yield(ReturnType val)
{
    std::cout << "GG yield" << std::endl;
    // called by the function
    _return_value = val;
    boost::context::jump_fcontext(_own_context, &_caller_context, 0);
}

template<typename ReturnType>
void generator<ReturnType>::context_function(intptr_t _this_intptr)
{
    std::cout << "GG in context_function" << std::endl;
    auto _this = (generator<ReturnType>*)(_this_intptr);

    try
    {
        _this->_return_value.reset();

        _this->_callable([_this](ReturnType val) { _this->yield(val); });
    }
    catch(...)
    {
        // TODO
        std::cout << "GG Exception in generator function" << std::endl;
    }

    // return back
    _this->_stopped = true;
    // jump back
    boost::context::jump_fcontext(_this->_own_context, &_this->_caller_context, 0);
}



// usage:
// generator<int> g = make_generator<int>([]()
// {
//      ...
//      yeld(7);
//      ...
//      yeld(42);
// }

};

#endif
