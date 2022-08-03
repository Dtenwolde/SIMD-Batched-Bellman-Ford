cmake_minimum_required(VERSION 3.22)
project(SIMD_Batched_Bellman_Ford)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(SIMD_Batched_Bellman_Ford
        main.cpp
        amd64cpuinfo.c
        )

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
