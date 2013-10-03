// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include "coroutines/globals.hpp"

#include "parallel.hpp"
#include "lzma_decompress.hpp"

#include <boost/filesystem.hpp>

#include <iostream>

//#include <stdio.h>
#include "coroutines_io/file.hpp"
#include "coroutines_io/globals.hpp"
#include "coroutines_io/service.hpp"


using namespace coroutines;
namespace bfs = boost::filesystem;

namespace torture {

static const unsigned BUFFERS = 4;
static const unsigned BUFFER_SIZE = 1024*1024;

/*
class file
{
public:

    file(const char* path, const char* mode)
    {
        _f = ::fopen(path, mode);
        if (!_f)
            throw std::runtime_error("Unable to open file");
    }

    ~file()
    {
        ::fclose(_f);
    }

    std::size_t read(void* buf, std::size_t max)
    {
        return ::fread(buf, 1, max, _f);
    }

    std::size_t write(void* buf, std::size_t size)
    {
        return ::fwrite(buf, size, 1, _f);
    }

private:

    FILE* _f = nullptr;
};
*/

void process_file(const bfs::path& in_path, const bfs::path& out_path);
void write_output(buffer_reader& decompressed, buffer_writer& decompressed_return, const bfs::path& output_file);
void read_input(buffer_writer& compressed, buffer_reader& compressed_return, const bfs::path& input_file);

// Main entry point
void parallel(const char* in, const char* out)
{

    scheduler sched(4 /*threads*/);
    set_scheduler(&sched);
    service serv(sched);
    set_service(&serv);
    serv.start();

    try
    {
        bfs::path input_dir(in);
        bfs::path output_dir(out);


        for(bfs::directory_iterator it(input_dir); it != bfs::directory_iterator(); ++it)
        {
            if (it->path().extension() == ".xz" && it->status().type() == bfs::regular_file)
            {
                bfs::path output_path = output_dir / it->path().filename().stem();
                go(std::string("proces_file ") + it->path().string(), process_file, it->path(), output_path);
            }

        }

    }
    catch(const std::exception& e)
    {
        std::cerr << "Error :" << e.what() << std::endl;
    }

    sched.wait();
    serv.stop();
    sched.wait();
    set_scheduler(nullptr);
}

void process_file(const bfs::path& input_file, const bfs::path& output_file)
{
    //std::cout << "process file: " << input_file << " -> " << output_file << std::endl;

    channel_pair<buffer> compressed = make_channel<buffer>(BUFFERS, "compressed");
    channel_pair<buffer> decompressed = make_channel<buffer>(BUFFERS, "decompressed");
    channel_pair<buffer> compressed_return = make_channel<buffer>(BUFFERS, "compressed_return");
    channel_pair<buffer> decompressed_return = make_channel<buffer>(BUFFERS, "decompressed_return");


    // start writer
    go(std::string("write_output ") + output_file.string(),
        write_output, std::move(decompressed.reader), std::move(decompressed_return.writer), output_file);

    // start decompressor
    go(std::string("lzma_decompress ") + input_file.string(),
        lzma_decompress,
        std::move(compressed.reader), std::move(compressed_return.writer),
        std::move(decompressed_return.reader), std::move(decompressed.writer));

    // read (in this coroutine)
    read_input(compressed.writer, compressed_return.reader, input_file);
}

void read_input(buffer_writer& compressed, buffer_reader& compressed_return, const bfs::path& input_file)
{
    try
    {
        //file f(input_file.string().c_str(), "rb");
        file f;
        f.open_for_reading(input_file.string());

        unsigned counter = 0;
        for(;;)
        {
            buffer b;
            if (counter++ < BUFFERS)
                b = buffer(BUFFER_SIZE);
            else
                b = compressed_return.get();
            std::size_t r = f.read(b.begin(), b.capacity());
            if (r == 0)
                break; // this will close the channel
            else
            {
                b.set_size(r);
                compressed.put(std::move(b));
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error reading file " << input_file << " : " << e.what() << std::endl;
    }
}


void write_output(buffer_reader& decompressed, buffer_writer& decompressed_return, const  bfs::path& output_file)
{
    std::cout << "write_output: " << output_file << std::endl;

    try
    {
        // open file
        //file f(output_file.string().c_str(), "wb");
        file f;
        f.open_for_writing(output_file.string());

        // fill the queue with allocated buffers
        for(unsigned i = 0; i < BUFFERS; i++)
        {
            decompressed_return.put(buffer(BUFFER_SIZE));
        }

        for(;;)
        {
            buffer b = decompressed.get();
            f.write(b.begin(), b.size());
            decompressed_return.put_nothrow(std::move(b));
        }
    }
    catch(const channel_closed&)
    {
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error writing to output file " << output_file << " : " << e.what() << std::endl;
    }
}

}
