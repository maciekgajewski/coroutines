find_package(Qt5Widgets)

include_directories(${Boost_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

add_executable(profiling_gui
    main.cpp
)


target_link_libraries(profiling_gui
    ${Qt5Widgets_LIBRARIES}

    profiling_reader
)
