// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>

#include "parallel.hpp"

#include <iostream>

// torture test:
// invocation: torture DIR
// how it works: it opens all files in the direcotry with .xz suffix, decompresses them and puts them into DIR/out with suffix removed
int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Error: directory missing. Invocation: torture INPUTDIR OUTPUTDIR" << std::endl;
        return 2;
    }

    torture::parallel(argv[1], argv[2]);
}

