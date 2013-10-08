find_package( Boost 1.54.0 COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(/home/maciej.gajewski/Software/cpp-netlib/http/src)
include_directories(/home/maciej.gajewski/Software/cpp-netlib/message/src)
link_directories(/home/maciej.gajewski/Software/cpp-netlib-build/http/src)
link_directories(/home/maciej.gajewski/Software/cpp-netlib-build/message/src)

add_executable(http_server
    server.cpp
    client_connection.cpp client_connection.hpp
)


target_link_libraries(http_server

    cppnetlib-http-message
    cppnetlib-uri
    cppnetlib-message

    coroutines
    coroutines_io

    ${Boost_LIBRARIES}
)
