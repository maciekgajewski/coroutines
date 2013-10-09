// Copyright (c) 2013 Maciej Gajewski
#ifndef COROUTINES_IO_SOCKET_STREAMBUF_HPP
#define COROUTINES_IO_SOCKET_STREAMBUF_HPP

#include "coroutines_io/base_pollable.hpp"

#include <array>
#include <streambuf>

namespace coroutines {

// current status:
// * no putback
class socket_istreambuf : public std::streambuf
{
public:

    using std::streambuf::int_type;
    using std::streambuf::traits_type;

    explicit socket_istreambuf(base_pollable& sock)
    : _socket(sock)
    {
        setg(end(), end(), end());
    }

    socket_istreambuf(const socket_istreambuf&) = delete;

protected:

    virtual int_type underflow() override
    {
        if (gptr() < egptr())
        {
            std::cout << "STREAM: buffer not exhausetd, why are you bothering me?" << std::endl;
            return traits_type::to_int_type(*gptr());
        }
        else
        {
            //std::cout << "STREAM: need to refill..." << std::endl;
            // need to load some data
            std::size_t n = _socket.read_some(begin(), BUFFER_SIZE);
            if (n == 0)
            {
                return traits_type::eof();
            }
            else
            {
                //std::cout << "STREAM: " << n << " bytes added to buffer" << std::endl;
                setg(begin(), begin(), begin() + n);
                return traits_type::to_int_type(*gptr());
            }
        }
    }

private:

    static constexpr unsigned BUFFER_SIZE = 4096;

    typedef std::array<char, BUFFER_SIZE> buffer_type;

    const char* begin() const { return _buffer.data(); }
    const char* end() const { return _buffer.data() + BUFFER_SIZE; }

    char* begin() { return _buffer.data(); }
    char* end() { return _buffer.data() + BUFFER_SIZE; }

    buffer_type _buffer;
    base_pollable& _socket;
};


class socket_ostreambuf: public std::streambuf
{
public:

    using std::streambuf::int_type;
    using std::streambuf::traits_type;

    explicit socket_ostreambuf(base_pollable& sock)
    : _socket(sock)
    {
        setp(begin(), end());
    }

    socket_ostreambuf(const socket_istreambuf&) = delete;

protected:

    virtual int_type overflow(int_type ch) override
    {
        _socket.write(begin(), pptr() - begin());
        if (ch != traits_type::eof())
        {
            _buffer[0] = traits_type::to_char_type(ch);
            setp(begin()+1, end());
        }
        else
        {
            setp(begin(), end());
        }
        return traits_type::to_int_type('a');
    }

    virtual int sync()
    {
        overflow(traits_type::eof());
        return 0;
    }

private:

    static constexpr unsigned BUFFER_SIZE = 4096;

    typedef std::array<char, BUFFER_SIZE> buffer_type;

    const char* begin() const { return _buffer.data(); }
    const char* end() const { return _buffer.data() + BUFFER_SIZE; }

    char* begin() { return _buffer.data(); }
    char* end() { return _buffer.data() + BUFFER_SIZE; }

    buffer_type _buffer;
    base_pollable& _socket;
};

}

#endif
