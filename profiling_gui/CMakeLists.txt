find_package(Qt5Widgets REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})

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
    globals.cpp globals.hpp
)


target_link_libraries(profiling_gui

    profiling_reader
)

qt5_use_modules(profiling_gui Widgets)
