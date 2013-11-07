#ifndef COROUTINES_LOCKING_COROUTINE_CHANNEL_HPP
#define COROUTINES_LOCKING_COROUTINE_CHANNEL_HPP

#include "coroutines/channel.hpp"
#include "coroutines/mutex.hpp"
#include "coroutines/condition_variable.hpp"
#include "coroutines/channel_closed.hpp"

#include <boost/format.hpp>

#include <memory>

namespace coroutines {

class scheduler;

// non-lock-free implementation
template<typename T>
class locking_channel
{
public:

    locking_channel(scheduler& _sched, std::size_t capacity, const std::string& name);
    locking_channel(const locking_channel&) = delete;
    ~locking_channel();

    class writer
    {
    public:
        writer(const std::shared_ptr<locking_channel>& impl)
            : _impl(impl) { }

        virtual void put(T v) { _impl->put(std::move(v)); }
        virtual void writer_close() { _impl->writer_close(); }
        virtual ~writer() { _impl->writer_close(); }

    private:

        std::shared_ptr<locking_channel> _impl;
    };

    class reader
    {
    public:
        reader(const std::shared_ptr<locking_channel>& impl)
            : _impl(impl) { }

        virtual T get() { return std::move(_impl->get()); }
        virtual bool try_get(T& b) { return _impl->try_get(b); }
        virtual void reader_close() { _impl->reader_close(); }
        virtual ~reader() { _impl->reader_close(); }

    private:

        std::shared_ptr<locking_channel> _impl;
    };

    // called by producer
    void put(T v);
    void writer_close() { do_close(); }

    // caled by consumer
    T get();
    bool try_get(T& b);
    void reader_close() { do_close(); }

private:

    void do_close();

    unsigned size() const
    {
        int s = _wr - _rd;
        if (s < 0)
            s += _capacity;
        //std::cout << "CHAN: _wr=" << _wr << ", _rd=" << _rd << ", size=" << s << std::endl;
        return (unsigned)s;
    }

    int wr_next() const
    {
        return (_wr + 1) % _capacity;
    }

    T* _data;
    int _rd = 0;
    int _wr = 0;
    std::size_t _capacity;
    condition_variable _producers_cv;
    condition_variable _consumers_cv;
    mutex _mutex;

    bool _closed = false;

    const std::string _read_checkpoint;
    const std::string _write_checkpoint;
};

template<typename T>
locking_channel<T>::locking_channel(scheduler& sched, std::size_t capacity, const std::string& name)
    : _data(static_cast<T*>(std::malloc(sizeof(T) * (capacity+1))))
    , _capacity(capacity+1)
    , _producers_cv(sched)
    , _consumers_cv(sched)
    , _mutex("channel mutex")

    , _read_checkpoint(name + " : reading")
    , _write_checkpoint(name + " : writing")
{
    assert(capacity >= 1);
    if (!_data)
    {
        throw std::bad_alloc();
    }
}

template<typename T>
locking_channel<T>::~locking_channel()
{
    // destroy anything that could still be in there
    int rd = _rd;
    int wr = _wr;
    while(rd != wr)
    {
        _data[rd].~T();
        rd = (rd+1) % _capacity;
    }
    std::free(_data);
}

template<typename T>
void locking_channel<T>::put(T v)
{
    std::lock_guard<mutex> lock(_mutex);

    _producers_cv.wait(_write_checkpoint, _mutex, [=]()// WARNING: the value of _wr & _rd may be different before and after waiting (modified by another threads)
    {
        return _rd != wr_next() || _closed;
    });

    if (_closed)
        throw channel_closed();

    new(&_data[_wr]) T(std::move(v));
    _wr = wr_next();

    //std::cout << boost::format("CHAN: after write, %d left in channel. _wr=%d, _rd=%d") % size() % _wr % _rd << std::endl;
    if (size() == 1)
        _consumers_cv.notify_all();
}

template<typename T>
T locking_channel<T>::get()
{
    std::lock_guard<mutex> lock(_mutex);

    _consumers_cv.wait(_read_checkpoint, _mutex, [=]() { return _rd != _wr || _closed; });

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

    //std::cout << "CHAN: after read, " << size() << " left in channel" << std::endl;
    if (size() == _capacity - 2)
        _producers_cv.notify_all();

    return v;
}

template<typename T>
bool locking_channel<T>::try_get(T& b)
{
    std::lock_guard<mutex> lock(_mutex);

    if (_rd == _wr)
    {
        return false;
    }
    else
    {
        b = std::move(_data[_rd]);
        _data[_rd].~T();
        _rd++;
        if (_rd == _capacity)
            _rd = 0;

        if (size() == _capacity - 2)
            _producers_cv.notify_all();

        return true;
    }
}


template<typename T>
void locking_channel<T>::do_close()
{
    std::lock_guard<mutex> lock(_mutex);
    _closed = true;
    _producers_cv.notify_all();
    _consumers_cv.notify_all();
}


}

#endif
