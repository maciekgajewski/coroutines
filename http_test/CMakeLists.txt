include_directories(/usr/local/cpp-netlib/include)
link_directories(/usr/local/cpp-netlib/lib)

add_executable(http_test_client
    client.cpp
)

target_link_libraries(http_test_client
    coroutines
    coroutines_io
    coroutines_http
)
