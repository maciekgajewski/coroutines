
add_library(coroutines STATIC	
    channel.hpp
    generator.hpp
    mutex.hpp
    threaded_channel.hpp
    threaded_scheduler.cpp threaded_scheduler.hpp
    globals.cpp globals.hpp
    spsc_queue.hpp
    coroutine.cpp coroutine.hpp
    coroutine_scheduler.cpp coroutine_scheduler.hpp
    context.cpp context.hpp
    thread_safe_queue.hpp
    monitor.cpp monitor.hpp
    condition_variable.hpp
    coroutine_channel.hpp
    locking_coroutine_channel.hpp
)
target_link_libraries(coroutines
    ${Boost_LIBRARIES}
    pthread
)
