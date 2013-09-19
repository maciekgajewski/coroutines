add_executable(test
        main.cpp	
	generator_tests.cpp generator_tests.hpp
)

target_link_libraries(test libcoroutines)
