find_package( Boost 1.54.0 COMPONENTS context)
include_directories(${Boost_INCLUDE_DIRS})

add_library(coroutines STATIC	
    algorithm.hpp
    channel.hpp
    condition_variable.hpp
    context.cpp context.hpp
    coroutine.cpp coroutine.hpp
    generator.hpp
    globals.cpp globals.hpp
    lock_free_channel.hpp
    locking_channel.hpp
    monitor.cpp monitor.hpp
    mutex.hpp
    scheduler.cpp scheduler.hpp
    spsc_queue.hpp
    thread_safe_queue.hpp
)

target_link_libraries(coroutines
    ${Boost_LIBRARIES}
    pthread
)
