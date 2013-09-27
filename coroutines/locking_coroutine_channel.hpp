#ifndef COROUTINES_LOCKING_COROUTINE_CHANNEL_HPP
#define COROUTINES_LOCKING_COROUTINE_CHANNEL_HPP

#include "channel.hpp"
#include "mutex.hpp"
#include "condition_variable.hpp"

namespace coroutines {

// non-lock-free implementation
template<typename T>
class locking_coroutine_channel : public i_writer_impl<T>, public i_reader_impl<T>
{
public:

    // factory
    static channel_pair<T> make(std::size_t capacity)
    {
        std::shared_ptr<locking_coroutine_channel<T>> me(new locking_coroutine_channel<T>(capacity));
        return channel_pair<T>(channel_reader<T>(me), channel_writer<T>(me));
    }

    locking_coroutine_channel(const locking_coroutine_channel&) = delete;
    virtual ~locking_coroutine_channel() override;

    // called by producer
    virtual void put(T v) override;
    virtual void writer_close() override { do_close(); }

    // caled by consumer
    virtual T get() override;
    virtual void reader_close() { do_close(); }


private:

    locking_coroutine_channel(std::size_t capacity);
    void do_close();

    T* _data;
    int _rd = 0;
    int _wr = 0;
    std::size_t _capacity;
    condition_variable _cv;
    mutex _mutex;

    bool _closed = false;
};

template<typename T>
locking_coroutine_channel<T>::locking_coroutine_channel(std::size_t capacity)
    : _data(static_cast<T*>(std::malloc(sizeof(T) * capacity+1)))
    , _capacity(capacity+1)
{
    assert(capacity >= 1);
    if (!_data)
    {
        throw std::bad_alloc();
    }
}

template<typename T>
locking_coroutine_channel<T>::~locking_coroutine_channel()
{
    // destroy anything that could still be in there
    int rd = _rd;
    int wr = _wr;
    while(rd != wr)
    {
        _data[rd].~T();
        rd = (rd+1) % _capacity;
    }
}

template<typename T>
void locking_coroutine_channel<T>::put(T v)
{
    std::lock_guard<mutex> lock(_mutex);

    int wr_next = _wr + 1;
    if (wr_next == _capacity)
        wr_next = 0;

    _cv.wait(_mutex, [=]() { return _rd != wr_next || _closed; });

    if (_closed)
        throw channel_closed();

    new(&_data[_wr]) T(std::move(v));
    _wr = wr_next;

    _cv.notify_one();
}

template<typename T>
T locking_coroutine_channel<T>::get()
{
    std::lock_guard<mutex> lock(_mutex);

    _cv.wait(_mutex, [=]() { return _rd != _wr || _closed; });

    if (_rd == _wr)
    {
        assert(_closed);
        throw channel_closed();
    }

    T v(std::move(_data[_rd]));
    _data[_rd].~T();
    _rd++;
    if (_rd == _capacity)
        _rd = 0;

    _cv.notify_one();

    return v;
}

template<typename T>
void locking_coroutine_channel<T>::do_close()
{
    std::lock_guard<mutex> lock(_mutex);
    _closed = true;
    _cv.notify_all();
}


}

#endif
