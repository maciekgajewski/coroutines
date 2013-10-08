#include "client_connection.hpp"

#include <network/message/wrappers/headers.hpp>
#include <network/constants.hpp>
#include <network/protocol/http/server/impl/parsers.ipp>
#include <boost/range.hpp>

#include <iostream>
#include <cstring>

//namespace network {
//namespace http {

//extern void parse_version(std::string const& partial_parsed,
//                          boost::fusion::tuple<uint8_t, uint8_t>& version_pair);
//extern void parse_headers(
//    std::string const& input,
//    std::vector<std::pair<std::string, std::string>>& container);

//}} // ns

client_connection::client_connection(tcp_socket&& s, handler_type&& handler)
    : _socket(std::move(s))
    , _handler(std::move(handler))
{
}


void client_connection::start()
{
    std::cout << "conenction from: " << _socket.remote_endpoint() << std::endl;

    std::ostringstream ip_stream;
    ip_stream << _socket.remote_endpoint().address().to_string() << ':'
              << _socket.remote_endpoint().port();
    request_.set_source(ip_stream.str());

    state_t state = method;

    buffer_type read_buffer;
    buffer_type::iterator new_start, data_end;

    new_start = read_buffer.begin();

    for(;;)
    {
        std::size_t bytes_transferred = _socket.read_some(read_buffer.data(), read_buffer.max_size());

        boost::logic::tribool parsed_ok;
        boost::iterator_range<buffer_type::iterator> result_range, input_range;
        data_end = read_buffer.begin();
        std::advance(data_end, bytes_transferred);

        switch (state) {
        case method:
            input_range = boost::make_iterator_range(new_start, data_end);
            boost::fusion::tie(parsed_ok, result_range) = parser_.parse_until(network::http::request_parser::method_done, input_range);
            if (!parsed_ok)
            {
                return client_error();
            }
            else if (parsed_ok == true)
            {
                std::string method;
                swap(partial_parsed, method);
                method.append(boost::begin(result_range), boost::end(result_range));
                boost::trim(method);
                request_.set_method(method);
                new_start = boost::end(result_range);
                // Determine whether we're going to need to parse the body of the
                // request. All we do is peek at the first character of the method
                // to determine whether it's a POST or a PUT.
                read_body_ = method.size() ? method[0] == 'P' : false;
            }
            else
            {
                partial_parsed.append(boost::begin(result_range),
                boost::end(result_range));
                new_start = read_buffer.begin();
                //read_more(method); // CONTINUE LOOP
                break;
            }
        case uri:
            input_range = boost::make_iterator_range(new_start, data_end);
            boost::fusion::tie(parsed_ok, result_range) = parser_.parse_until(network::http::request_parser::uri_done, input_range);
            if (!parsed_ok) {
              return client_error();
            } else if (parsed_ok == true) {
              std::string destination;
              swap(partial_parsed, destination);
              destination.append(boost::begin(result_range),
              boost::end(result_range));
              boost::trim(destination);
              request_.set_destination(destination);
              new_start = boost::end(result_range);
            } else {
              partial_parsed.append(boost::begin(result_range),
              boost::end(result_range));
              new_start = read_buffer.begin();
              //read_more(uri);
              state = uri; // CONTINUE LOOP
              break;
            }
        case version:
            input_range = boost::make_iterator_range(new_start, data_end);
            boost::fusion::tie(parsed_ok, result_range) =
            parser_.parse_until(network::http::request_parser::version_done, input_range);
            if (!parsed_ok) {
              return client_error();
            } else if (parsed_ok == true) {
              boost::fusion::tuple<uint8_t, uint8_t> version_pair;
              partial_parsed.append(boost::begin(result_range),
              boost::end(result_range));
              network::http::parse_version(partial_parsed, version_pair);
              request_.set_version_major(boost::fusion::get<0>(version_pair));
              request_.set_version_minor(boost::fusion::get<1>(version_pair));
              new_start = boost::end(result_range);
              partial_parsed.clear();
            } else {
              partial_parsed.append(boost::begin(result_range),
              boost::end(result_range));
              new_start = read_buffer.begin();
              //read_more(version);
              state = version; // CONTINUE LOOP
              break;
            }
        case headers:
            input_range = boost::make_iterator_range(new_start, data_end);
            boost::fusion::tie(parsed_ok, result_range) =
            parser_.parse_until(network::http::request_parser::headers_done, input_range);
            if (!parsed_ok)
            {
                return client_error();
            }
            else if (parsed_ok == true)
            {
                partial_parsed.append(boost::begin(result_range), boost::end(result_range));
                std::vector<std::pair<std::string, std::string>> headers;
                network::http::parse_headers(partial_parsed, headers);

                for (std::vector<
                std::pair<std::string, std::string>>::const_iterator it =
                headers.begin();
                it != headers.end(); ++it) {
                  request_.append_header(it->first, it->second);
                }
                new_start = boost::end(result_range);
                if (read_body_) {} else {
                    network::http::response response_;
                    _handler(request_, response_);
                    flatten_response();

                    for(buffer_type& buffer: output_buffers_)
                    {
                        _socket.write(buffer.data(), buffer.size());
                    }
                    /*
                    std::vector<boost::asio::const_buffer> response_buffers(output_buffers_.size());
                    std::transform(
                        output_buffers_.begin(),
                        output_buffers_.end(),
                        response_buffers.begin(),
                          [](buffer_type const& buffer) {
                              return boost::asio::const_buffer(buffer.data(), buffer.size());
                          });
                  boost::asio::async_write(
                  socket_,
                  response_buffers,
                  wrapper_.wrap(
                  std::bind(&sync_server_connection::handle_write,
                  sync_server_connection::shared_from_this(),
                  boost::asio::placeholders::error)));
                  */
                  // TODO write reponse here
                }
                return;
            } else {
              partial_parsed.append(boost::begin(result_range),
              boost::end(result_range));
              new_start = read_buffer.begin();
              state = headers; // CONTINUE LOOP
              break;
            }
        default:
          BOOST_ASSERT(
          false &&
          "This is a bug, report to the cpp-netlib devel mailing list!");
          std::abort();
        }
    } // main loop
}

void client_connection::client_error()
{
    static char const bad_request[] =
        "HTTP/1.0 400 Bad Request\r\nConnection: close\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nBad Request.";

    _socket.write(bad_request, std::strlen(bad_request));
    _socket.shutdown();
}

void client_connection::flatten_response()
{
    std::uint16_t status = network::http::status(response_);
    std::string status_message = network::http::status_message(response_);
    network::headers_wrapper::container_type headers = network::headers(response_);
    std::ostringstream status_line;
    status_line << status << network::constants::space() << status_message
                << network::constants::space() << network::constants::http_slash()
                << "1.1"  // TODO: make this a constant
                << network::constants::crlf();
    segmented_write(status_line.str());
    std::ostringstream header_stream;
    auto it = std::begin(headers), end = std::end(headers);
    for (; it != end; ++it) {
      const auto& header = *it;
      //for (auto const &header : headers) {
      header_stream << header.first << network::constants::colon() << network::constants::space()
                    << header.second << network::constants::crlf();
    }
    header_stream << network::constants::crlf();
    segmented_write(header_stream.str());
    bool done = false;
    while (!done)
    {
        buffer_type buffer;
        response_.get_body(
            [&done, &buffer](std::string::const_iterator start, size_t length)
            {
                if (!length)
                     done = true;
                else
                {
                    std::string::const_iterator past_end = start;
                    std::advance(past_end, length);
                    std::copy(start, past_end, buffer.begin());
                }
            },
            buffer.size());

        if (!done)
            output_buffers_.emplace_back(std::move(buffer));
    }
}

void client_connection::segmented_write(std::string data)
{
    while (!boost::empty(data))
    {
        buffer_type buffer;
        auto end = std::copy_n(boost::begin(data), buffer.size(), buffer.begin());
        data.erase(0, std::distance(buffer.begin(), end));
        output_buffers_.emplace_back(std::move(buffer));
    }
}
