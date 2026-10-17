cmake_minimum_required(VERSION 3.20)

project(WVectorDB VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(wvectordb STATIC
    src/collection.cpp
)

target_include_directories(wvectordb
    PUBLIC
        include
)

if(MSVC)
    target_compile_options(wvectordb PRIVATE /W4 /permissive-)
else()
    target_compile_options(wvectordb PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(wvectordb_smoke
    tests/smoke_test.cpp
)

target_link_libraries(wvectordb_smoke PRIVATE wvectordb)

add_executable(wvectordb_benchmark
    benchmark/benchmark.cpp
)

target_link_libraries(wvectordb_benchmark PRIVATE wvectordb)

add_executable(wvectordb_batch_benchmark
    benchmark/batch_benchmark.cpp
)

target_link_libraries(wvectordb_batch_benchmark PRIVATE wvectordb)

add_executable(wvectordb_basic_search
    examples/basic_search.cpp
)

target_link_libraries(wvectordb_basic_search PRIVATE wvectordb)

if(MSVC)
    target_compile_options(wvectordb_smoke PRIVATE /W4 /permissive-)
    target_compile_options(wvectordb_benchmark PRIVATE /W4 /permissive-)
    target_compile_options(wvectordb_batch_benchmark PRIVATE /W4 /permissive-)
    target_compile_options(wvectordb_basic_search PRIVATE /W4 /permissive-)
else()
    target_compile_options(wvectordb_smoke PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(wvectordb_benchmark PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(wvectordb_batch_benchmark PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(wvectordb_basic_search PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME phase5_collection_batch_api COMMAND wvectordb_smoke)
add_test(NAME phase5_basic_search_example COMMAND wvectordb_basic_search)
