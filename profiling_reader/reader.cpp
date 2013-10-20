// Copyright (c) 2013 Maciej Gajewski
#include "profiling_reader/reader.hpp"

#include <boost/lexical_cast.hpp>

#include <utility>

namespace profiling_reader {

template<typename T>
void read_until(std::ifstream& file, T& o, char delim)
{
    static const unsigned bufsize = 128;
    char buf[bufsize];
    file.getline(buf, bufsize, delim);
    o = boost::lexical_cast<T>(buf);
}

reader::reader(const std::string& file_name)
{
    std::ifstream file(file_name);
    file.exceptions(std::ios_base::eofbit);
    try
    {
        while(!file.eof())
        {
            record_type record;

            read_until(file, record.time, ',');
            read_until(file, record.thread_id, ',');
            read_until(file, record.object_type, ',');
            read_until(file, record.object_id, ',');
            read_until(file, record.event, ',');
            read_until(file, record.data, '\n');

            if (record.event == "clock calibration")
            {
                _ticks_per_ns = boost::lexical_cast<double>(record.data);
            }

            _by_time.insert(std::make_pair(record.time, record));
        }
    }
    catch(const std::ios_base::failure&)
    {
    }
}

} // namespace profiling
