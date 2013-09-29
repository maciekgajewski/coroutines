project(coroutines)
cmake_minimum_required(VERSION 2.8)

include_directories(${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(coroutines)
add_subdirectory(coroutines_io)
add_subdirectory(test)
add_subdirectory(torture)
