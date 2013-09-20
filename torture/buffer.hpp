// Copyright (c) 2013 Maciej Gajewski
#ifndef TORTURE_BUFFER_HPP
#define TORTURE_BUFFER_HPP

#include <memory>
#include <utility>

namespace torture
{

// used to send blocks of data around. Movable, bot not copytable
class buffer
{
public:

    typedef char value_type;
    typedef char* iterator;
    typedef const char* const_iterator;

    // null buffer
    buffer() : _capacity(0), _size(0)
    { }

    // alocated buffer
    buffer(std::size_t capacity)
    : _capacity(capacity), _size(0), _data(new char[_capacity])
    { }

    buffer(const buffer&) = delete;
    buffer(buffer&& o) noexcept
    {
        swap(o);
    }

    buffer& operator=(buffer&& o)
    {

    }

    // iterators
    iterator begin() { return _data.get(); }
    iterator end() { return _data.get() + _size; }
    const_iterator begin() const { return _data.get(); }
    const_iterator end() const { return _data.get() + _size; }

    // size/capacity
    void set_size(std::size_t s) { _size = s; }
    std::size_t size() const { return _size; }
    std::size_t capacity() const { return _capacity; }

    bool is_null() const { return !_capacity; }

    // other
    void swap(buffer& o) noexcept
    {
        std::swap(_capacity, o._capacity);
        std::swap(_size, o._size);
        std::swap(_data, o._data);
    }

private:

    std::size_t _capacity;  // buffer capacity
    std::size_t _size;      // amount of data in
    std::unique_ptr<char[]> _data;
};

inline void swap(buffer& a, buffer& b) noexcept
{
    a.swap(b);
}

}

#endif
