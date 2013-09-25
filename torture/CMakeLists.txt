add_executable(torture
        main.cpp
        parallel.cpp parallel.hpp
        buffer.hpp
        lzma_decompress.hpp lzma_decompress.cpp
)

target_link_libraries(torture
    coroutines

    ${Boost_LIBS}
    lzma
)
