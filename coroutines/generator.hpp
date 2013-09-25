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

// exception thrown where generator function exists
struct generator_finished : public std::exception
{
    virtual const char* what() const noexcept { return "generator finished"; }
};

template<typename ReturnType>
class generator
{
public:

    typedef std::function<void(const ReturnType&)> yield_function_type;
    typedef std::function<void(yield_function_type)> generator_function_type;

    // former 'init()'
    generator(generator_function_type generator, std::size_t stack_size = DEFAULT_STACK_SIZE)
        : _generator(std::move(generator))
    {
        // allocate stack for new context
        _stack = new char[stack_size];

        // make a new context. The returned fcontext_t is created on the new stack, so there is no need to delete it
        _new_context = boost::context::make_fcontext(
                    _stack + stack_size, // new stack pointer. On x86/64 it hast be the TOP of the stack (hence the "+ STACK_SIZE")
                    stack_size,
                    &generator::static_generator_function); // will call generator wrapper
    }

    // prevent copying
    generator(const generator&) = delete;

    // former 'cleanup()'
    ~generator()
    {
        delete _stack;
        _stack = nullptr;
        _new_context = nullptr;
    }

    ReturnType next()
    {
        // prevent calling when the generator function already finished
        if (_exception)
            std::rethrow_exception(_exception);

        // switch to function context. May set _exception
        boost::context::jump_fcontext(&_main_context, _new_context, reinterpret_cast<intptr_t>(this));
        if (_exception)
            std::rethrow_exception(_exception);
        else
            return *_return_value;
    }

private:

    // former global variables
    boost::context::fcontext_t _main_context; // will hold the main execution context
    boost::context::fcontext_t* _new_context = nullptr; // will point to the new context
    static const int DEFAULT_STACK_SIZE= 64*1024; // completely arbitrary value
    char* _stack = nullptr;

    generator_function_type _generator; // generator function

    std::exception_ptr _exception = nullptr;// pointer to exception thrown by generator function
    boost::optional<ReturnType> _return_value; // optional allows for using typed without defautl constructor


    // the actual generator function used to create context
    static void static_generator_function(intptr_t param)
    {
        generator* _this = reinterpret_cast<generator*>(param);
        _this->generator_wrapper();
    }

    void yield(const ReturnType& value)
    {
        _return_value = value;
        boost::context::jump_fcontext(_new_context, &_main_context, 0); // switch back to the main context
    }

    void generator_wrapper()
    {
        try
        {
            _generator([this](const ReturnType& value) // use lambda to bind this to yield
            {
                yield(value);
            });
            throw generator_finished();
        }
        catch(...)
        {
            // store the exception, is it can be thrown back in the main context
            _exception = std::current_exception();
            boost::context::jump_fcontext(_new_context, &_main_context, 0); // switch back to the main context
        }
    }
};

}

#endif
