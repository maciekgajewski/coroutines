find_package( Boost 1.54.0 COMPONENTS unit_test_framework)

add_definitions(-DBOOST_TEST_DYN_LINK)

add_executable(test
    main.cpp
    fixtures.cpp fixtures.hpp

    generator_tests.cpp generator_tests.hpp

    channel_tests.cpp
    scheduler_tests.cpp
    mutex_tests.cpp
)

target_link_libraries(test
    ${Boost_LIBRARIES}

    coroutines
)
