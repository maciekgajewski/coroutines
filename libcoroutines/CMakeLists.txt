
add_library(libcoroutines STATIC	
		channel.hpp
		generator.hpp
		mutex.hpp
		threaded_channel.hpp
		threaded_scheduler.cpp threaded_scheduler.hpp
)
target_link_libraries(libcoroutines ${Boost_LIBRARIES})
