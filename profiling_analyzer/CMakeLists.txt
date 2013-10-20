include_directories(${Boost_INCLUDE_DIRS})

add_executable(profiling_analyzer
    main.cpp
)


target_link_libraries(profiling_analyzer

    profiling_reader
)
