find_package( Boost 1.54.0 COMPONENTS system regex date_time)
include_directories(${Boost_INCLUDE_DIRS})

add_library(coroutines_http STATIC
    http_client.hpp http_client.cpp
)

include_directories(/usr/local/cpp-netlib/include)
link_directories(/usr/local/cpp-netlib/lib)

target_link_libraries(coroutines_http

    cppnetlib-http-message
    cppnetlib-uri
    cppnetlib-message

    coroutines
    coroutines_io
    ${Boost_LIBRARIES}
)
