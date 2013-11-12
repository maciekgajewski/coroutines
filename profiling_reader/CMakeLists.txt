include_directories(${Boost_INCLUDE_DIRS})

add_library(profiling_reader STATIC
    reader.cpp reader.hpp
)
