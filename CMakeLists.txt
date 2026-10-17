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

if(MSVC)
    target_compile_options(wvectordb_smoke PRIVATE /W4 /permissive-)
else()
    target_compile_options(wvectordb_smoke PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME phase3_collection_search COMMAND wvectordb_smoke)
