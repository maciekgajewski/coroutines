// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

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
            int r = generator.get();
            std::cout << "generator returned: " << r << std::endl;
        }
    }
    catch(const std::exception& e)
    {
        std::cout << "generator threw: " << e.what() << std::endl;
    }

    std::cout << std::boolalpha << "generator stopped? : " << generator.stopped() << std::endl;

    std::cout << "one more call, exception expected..." << std::endl;
    try
    {
        int r = generator.get();
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
                generator.get();
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
