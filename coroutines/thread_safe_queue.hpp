// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_THREAD_SAFE_QUEUE_HPP
#define COROUTINES_THREAD_SAFE_QUEUE_HPP

#include "mutex.hpp"

#include <boost/optional.hpp>

#include <list>


namespace coroutines {

template<typename T>
class thread_safe_queue
{
public:

    thread_safe_queue() = default;
    thread_safe_queue(const thread_safe_queue&) = delete;

    thread_safe_queue(thread_safe_queue&& o)
    {
        swap(o);
    }

    bool pop(T& b)
    {
        std::lock_guard<mutex> lock(_mutex);
        if (!_data.empty())
        {
            b = std::move(_data.front());
            _data.pop_front();
            return true;
        }
        return false;
    }

    void get_all(std::list<T>& out)
    {
        std::lock_guard<mutex> lock(_mutex);
        out.splice(out.end(), _data);
    }

    void push(T&& v)
    {
        std::lock_guard<mutex> lock(_mutex);
        _data.push_back(std::move(v));
    }

    void push(std::list<T>& in)
    {
        std::lock_guard<mutex> lock(_mutex);
        _data.splice(_data.end(), in);
    }

    void swap(thread_safe_queue<T>& o) noexcept
    {
        std::lock(_mutex, o._mutex);
        std::swap(_data, o._data);
        _mutex.unlock();
        o._mutex.unlock();
    }

    bool empty() const
    {
        return _data.empty();
    }

private:

    std::list<T> _data;
    mutex _mutex;
};

}

namespace std
{

template<typename T>
void swap(::coroutines::thread_safe_queue<T>& a, ::coroutines::thread_safe_queue<T>& b)
{
    a.swap(b);
}

}

#endif
