#ifndef COROUTINES_COROUTINE_CHANNEL_HPP
#define COROUTINES_COROUTINE_CHANNEL_HPP

#include "channel.hpp"
#include "spsc_queue.hpp"
#include "monitor.hpp"

namespace coroutines {


// DOES NOT WORK
// it may work if two mutexes would be replace by single reader/writer lock
// to be revisited
template<typename T>
class lock_free_channel : public i_writer_impl<T>, public i_reader_impl<T>
{
public:

    // factory
    static channel_pair<T> make(std::size_t capacity)
    {
        std::shared_ptr<lock_free_channel<T>> me(new lock_free_channel<T>(capacity));
        return channel_pair<T>(channel_reader<T>(me), channel_writer<T>(me));
    }

    lock_free_channel(const lock_free_channel&) = delete;

    // called by producer
    virtual void put(T v) override;
    virtual void writer_close() override { _closed = true; }

    // caled by consumer
    virtual T get() override;
    virtual void reader_close() { _closed = true; }


private:

    lock_free_channel(std::size_t capacity);
    void do_close();

    spsc_queue<T> _queue;

    monitor _reader_monitor, _writer_monitor;

    volatile bool _closed = false;
};

template<typename T>
lock_free_channel<T>::lock_free_channel(std::size_t capacity)
    : _queue(capacity+1)
{

}

template<typename T>
void lock_free_channel<T>::put(T v)
{
    while(!_queue.put(v) && !_closed)
    {
        _writer_monitor.wait();
    }

    if (_closed)
        throw channel_closed();

    _reader_monitor.wake_one();
}

template<typename T>
T lock_free_channel<T>::get()
{
    T v;
    bool success = false;
    while(!(success = _queue.get(v)) && !_closed)
    {
        _reader_monitor.wait();
    }

    if (!success)
    {
        assert(_closed);
        throw channel_closed();
    }

    _writer_monitor.wake_one();
    return v;
}



}

#endif
