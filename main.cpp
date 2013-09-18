// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include "channel.hpp"
#include "threaded_channel.hpp"

#include <iostream>

int main(int , char** )
{
    // create channel
    coroutines::channel_pair<int> pair(coroutines::threaded_channel<int>::make(10));

    int val = 7;
    std::cout << "channel created, writing " << val << std::endl;
    pair.writer.put(val);

    std::cout << "reading ..." << std::endl;
    int r = pair.reader.get();
    std::cout << "read: " << r << std::endl;

}

