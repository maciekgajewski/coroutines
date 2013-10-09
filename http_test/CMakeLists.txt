find_package( Boost 1.54.0 COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})


add_executable(http_server
    server.cpp
    client_connection.cpp client_connection.hpp
)


target_link_libraries(http_server

    coroutines
    coroutines_io

    ${Boost_LIBRARIES}
)
