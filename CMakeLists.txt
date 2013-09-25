project(coroutines)
cmake_minimum_required(VERSION 2.8)

find_package( Boost 1.54.0 COMPONENTS context filesystem system)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(coroutines)
add_subdirectory(test)
add_subdirectory(torture)
