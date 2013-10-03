find_package( Boost 1.38.0 COMPONENTS filesystem system)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(torture
        main.cpp
        parallel.cpp parallel.hpp
        buffer.hpp
        lzma_decompress.hpp lzma_decompress.cpp
)

target_link_libraries(torture
    coroutines

    ${Boost_LIBRARIES}
    lzma
)
