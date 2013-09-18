// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include <boost/context/all.hpp>
#include <iostream>
#include <functional>
#include <exception>

// exception thrown where generator function exists
struct generator_finished : public std::exception
{
    virtual const char* what() const noexcept { return "generator finished"; }
};

class generator
{
public:

    typedef std::function<void(intptr_t)> yield_function_type;
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

    // former 'cleanup()'
    ~generator()
    {
        delete _stack;
        _stack = nullptr;
        _new_context = nullptr;
    }

    intptr_t next()
    {
        // prevent calling when the generator function already finished
        if (_exception)
            std::rethrow_exception(_exception);

        // switch to function context. May set _exception
        intptr_t result = boost::context::jump_fcontext(&_main_context, _new_context, reinterpret_cast<intptr_t>(this));
        if (_exception)
            std::rethrow_exception(_exception);
        else
            return result;
    }

private:

    // former global variables
    boost::context::fcontext_t _main_context; // will hold the main execution context
    boost::context::fcontext_t* _new_context = nullptr; // will point to the new context
    static const int DEFAULT_STACK_SIZE= 64*1024; // completely arbitrary value
    char* _stack = nullptr;

    generator_function_type _generator; // generator function

    std::exception_ptr _exception = nullptr;// pointer to exception thrown by generator function


    // the actual generator function used to create context
    static void static_generator_function(intptr_t param)
    {
        generator* _this = reinterpret_cast<generator*>(param);
        _this->generator_wrapper();
    }

    void yield(intptr_t value)
    {
        boost::context::jump_fcontext(_new_context, &_main_context, value); // switch back to the main context
    }

    void generator_wrapper()
    {
        try
        {
            _generator([this](intptr_t value) // use lambda to bind this to yield
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

void fibonacci(const generator::yield_function_type& yield)
{
    intptr_t last = 1;
    intptr_t current = 1;
    for(;;)
    {
        yield(current);
        intptr_t nxt = last + current;
        last = current;
        current = nxt;
    }
}

int main(int , char** )
{
    const int N = 10;
    std::cout << "Two fibonacci sequences generated in parallel::" << std::endl;
    generator generator1(fibonacci);
    std::cout << "seq #1: " << generator1.next() << std::endl;
    std::cout << "seq #1: " << generator1.next() << std::endl;

    generator generator2(fibonacci);
    for(int i = 0; i < N; i++)
    {
        std::cout << "seq #1: " << generator1.next() << std::endl;
        std::cout << "seq #2:       " << generator2.next() << std::endl;
    }
}


#if 0
//#include "naive_scheduler.hpp"
#include "generator.hpp"

#include <iostream>
#include <thread>
#include <chrono>

/*
void fun(corountines::naive_channel<int> ch)
{
    std::cout << "boo" << std::endl;

    int x = ch.get();
    std::cout << "received: " << x << std::endl;
}

void fun2(int x, corountines::naive_channel<int> ch)
{
    std::cout << "x = " << x << std::endl;
    int y = ch.get();
    std::cout << "fun2, received: " << y << std::endl;
}

void test_naive_corountines()
{
    corountines::naive_scheduler scheduler;

    auto int_channel = scheduler.make_channel<int>(3);

    scheduler.go(fun, int_channel);
    scheduler.go(fun2, 42, int_channel);
    std::cout << "Hello" << std::endl;
    std::cout << "sending ints..." << std::endl;
    int_channel.put(1);
    int_channel.put(2);
}
*/

void gen_fun1(std::function<void (int)> yield)
{
    for(int i = 0; i < 10; ++i)
    {
        yield(i);
    }
}

void gen_fun2(std::function<void (int)> yield)
{
    for(int i = 0; i < 4; ++i)
    {
        yield(i*2);
    }
    throw std::runtime_error("Exception in generator function");
}

template<typename GeneratorType>
void test_generator(GeneratorType& generator)
{
    try
    {
        for(;;)
        {
            int r = generator();
            std::cout << "generator returned: " << r << std::endl;
        }
    }
    catch(const std::exception& e)
    {
        std::cout << "generator threw: " << e.what() << std::endl;
    }

    std::cout << std::boolalpha << "generator finished? : " << generator.finished() << std::endl;

    std::cout << "one more call, exception expected..." << std::endl;
    try
    {
        int r = generator();
        std::cout << "unexpected: generator returned: " << r << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cout << "expected: generator threw: " << e.what() << std::endl;
    }
}

template<typename GeneratorType>
void benchmark_generator(GeneratorType& generator)
{
    const int repetitions = 1000000;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < repetitions; i++)
    {
        try
        {
            for(;;)
            {
                generator();
            }
        }
        catch(const std::exception& e)
        {
            break;
        }
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << "becnhmark result: rep: " << repetitions << ", duration: " << (end-start)/std::chrono::nanoseconds(1) << " ns" << std::endl;
}



int main(int , char** )
{
    std::cout << "main start" << std::endl;
    // test
    {
        auto generator1 = corountines::make_generator<int>(gen_fun1);
        auto generator2 = corountines::make_generator<int>(gen_fun2);

        std::cout << "test generator 1:" << std::endl;
        test_generator(generator1);

        std::cout << "test generator 2:" << std::endl;
        test_generator(generator2);
    }

    // benchmark
    /*
    {
        auto generator1 = corountines::make_generator<int>(gen_fun1);
        auto generator2 = corountines::make_generator<int>(gen_fun2);

        std::cout << "becnhmarking generator 1..." << std::endl;
        benchmark_generator(generator1);
        std::cout << "becnhmarking generator 2..." << std::endl;
        benchmark_generator(generator2);
    }
    */
}
#endif
