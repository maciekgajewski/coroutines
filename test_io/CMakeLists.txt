add_executable(test_io
    main.cpp
)

target_link_libraries(test_io
    coroutines
    coroutines_io
)
