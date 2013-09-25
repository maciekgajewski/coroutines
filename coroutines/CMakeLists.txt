
add_library(coroutines STATIC	
		channel.hpp
		generator.hpp
		mutex.hpp
		threaded_channel.hpp
		threaded_scheduler.cpp threaded_scheduler.hpp
                globals.cpp globals.hpp
                spsc_queue.hpp
)
target_link_libraries(coroutines
    ${Boost_LIBRARIES}
    pthread
)
