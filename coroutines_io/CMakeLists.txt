find_package( Boost 1.54.0 COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})

add_library(coroutines_io STATIC
    buffer.hpp
    globals.cpp globals.hpp
    service.cpp service.hpp
    tcp_socket.cpp tcp_socket.hpp
    tcp_acceptor.cpp tcp_acceptor.hpp
    base_pollable.cpp base_pollable.hpp
    file.cpp file.hpp
    tcp_resolver.cpp tcp_resolver.hpp

    detail/poller.cpp detail/poller.hpp
)

target_link_libraries(coroutines_io

    coroutines
    ${Boost_LIBRARIES}
)
