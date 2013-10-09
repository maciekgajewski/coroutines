#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include "Poco/Net/HTTPRequest.h"

class http_request : public Poco::Net::HTTPRequest
{
public:
    http_request(std::istream& stream)
    : _stream(stream)
    {
    }

    void read_header()
    {
        read(_stream);
    }

    // stream for reading body
    std::istream& stream() { return _stream; }

private:

    std::istream& _stream;
};

#endif // HTTP_REQUEST_HPP
