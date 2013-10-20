find_package( Boost 1.54.0 COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})

if(ENABLE_PROFILING)
    message(STATUS "Profiling enabled")
    add_definitions(-DCOROUTINES_PROFILING)
endif()

add_executable(http_server
    server.cpp
    client_connection.cpp client_connection.hpp
    http_response.hpp
    http_request.hpp

)


target_link_libraries(http_server

    coroutines
    coroutines_io

    PocoNet

    ${Boost_LIBRARIES}
)
