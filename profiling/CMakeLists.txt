include_directories(${Boost_INCLUDE_DIRS})

add_library(profiling STATIC
    profiling.cpp profiling.hpp
)

#target_link_libraries(profiling
#)
