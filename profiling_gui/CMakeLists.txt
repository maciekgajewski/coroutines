find_package(Qt5Widgets REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt5_wrap_ui(UI_FILES mainwindow.ui)

add_executable(profiling_gui
    ${UI_FILES}

    main.cpp
    mainwindow.cpp mainwindow.hpp
    flowdiagram.cpp flowdiagram.hpp
    flowdiagram_items.cpp flowdiagram_items.hpp
    horizontalview.cpp horizontalview.hpp
    coroutinesmodel.cpp coroutinesmodel.hpp
)


target_link_libraries(profiling_gui
#    ${Qt5Widgets_LIBRARIES}

    profiling_reader
)

qt5_use_modules(profiling_gui Widgets)
