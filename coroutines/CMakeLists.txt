find_package( Boost 1.54.0 COMPONENTS context system)
include_directories(${Boost_INCLUDE_DIRS})

if(ENABLE_PROFILING)
    message(STATUS "Profiling enabled")
    add_definitions(-DCOROUTINES_PROFILING)
    set(PROFILING_LIB profiling)

    if(ENABLE_SPINLOCK_PROFILING)
        message(STATUS "Spinlock profiling enabled")
        add_definitions(-DCOROUTINES_SPINLOCKS_PROFILING)
    endif()

endif()


add_library(coroutines STATIC
    algorithm.hpp
    channel.hpp
    channel_closed.hpp
    condition_variable.hpp
    coroutine.cpp coroutine.hpp
    generator.hpp
    globals.cpp globals.hpp
    lock_free_channel.hpp
    locking_channel.hpp
    monitor.cpp monitor.hpp
    mutex.hpp
    scheduler.cpp scheduler.hpp
    spsc_queue.hpp
    logging.hpp
    processor.cpp processor.hpp
    processor_container.cpp processor_container.hpp
)

target_link_libraries(coroutines
    ${PROFILING_LIB}
    ${Boost_LIBRARIES}
    pthread
)


