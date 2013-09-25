// Copyright (c) 2013 Maciej Gajewski
#ifndef TORTURE_LZMA_DECOMPRESS_HPP
#define TORTURE_LZMA_DECOMPRESS_HPP

#include "buffer.hpp"

#include "coroutines/channel.hpp"

namespace torture {

typedef coroutines::channel_reader<buffer> buffer_reader;
typedef coroutines::channel_writer<buffer> buffer_writer;

// function that decopresses LZMA stream.
// Operation:
// Receives buffers with compressed data via compressed until channel closes, returns bufers trough compressed_return.
// Receives buffers for decompressed data from decompressed_return, sends compressed back trough decopressed, closes chanel when done.
void lzma_decompress(
    buffer_reader& compressed,
    buffer_writer& compressed_return,

    buffer_reader& decompressed_return,
    buffer_writer& decopressed
);

}

#endif
