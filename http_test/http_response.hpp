#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "Poco/Net/HTTPResponse.h"

class http_response : public Poco::Net::HTTPResponse
{
public:
    http_response(std::ostream& stream)
    : _stream(stream)
    { }

    // header manipulation has no effect after this call
    std::ostream& stream()
    {
        ensure_header_send();
        return _stream;
    }

    ~http_response()
    {
        ensure_header_send();
        _stream.flush();
    }

private:

    void ensure_header_send()
    {
        if (!_header_written)
        {
            write(_stream);
            _header_written = true;
        }
    }

     std::ostream& _stream;
    bool _header_written = false;
};

#endif // HTTP_REPLY_HPP
