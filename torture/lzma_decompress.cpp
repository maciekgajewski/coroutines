// Copyright (c) 2013 Maciej Gajewski

#include "lzma_decompress.hpp"

#include <lzma.h>

#include <iostream>

namespace torture {

void lzma_decompress(
    buffer_reader& compressed,
    buffer_writer& compressed_return,

    buffer_reader& decompressed_return,
    buffer_writer& decopressed
)
{
    lzma_stream stream = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_stream_decoder(&stream, UINT64_MAX, LZMA_CONCATENATED);
    if (ret != LZMA_OK)
    {
        throw std::runtime_error("lzma initialization failed");
    }

    buffer inbuf;
    buffer outbuf = decompressed_return.get(); // get allocated buffer from writer

    stream.next_in = nullptr;
    stream.avail_in = 0;
    stream.next_out = (unsigned char*)outbuf.begin();
    stream.avail_out = outbuf.capacity();


    while(ret == LZMA_OK)
    {
        lzma_action action = LZMA_RUN;

        // read more data, if input buffer empty
        if(stream.avail_in == 0)
        {
            // return previous used buffer
            if (!inbuf.is_null())
                compressed_return.put_nothrow(std::move(inbuf));
            try
            {
                // read one
                inbuf = compressed.get();
                stream.next_in = (unsigned char*)inbuf.begin();
                stream.avail_in = inbuf.size();
            }
            catch(const coroutines::channel_closed&)
            {
                action = LZMA_FINISH;
            }
        }

        // decompress
        ret = lzma_code(&stream, action);
        if (stream.avail_out == 0 || ret == LZMA_STREAM_END)
        {
            outbuf.set_size(stream.next_out - (unsigned char*)outbuf.begin());
            // send the buffer, receive an empty one
            decopressed.put(std::move(outbuf));

            if (ret != LZMA_STREAM_END)
            {
                outbuf = decompressed_return.get();
                stream.next_out = (unsigned char*)outbuf.begin();
                stream.avail_out = outbuf.capacity();
            }
        }

    }

    lzma_end(&stream);

    if (ret != LZMA_STREAM_END)
    {
        std::cerr << "lzma decoding error" << std::endl;
    }
    // exit will close all channels
}


}



