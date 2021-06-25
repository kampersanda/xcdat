cmake_minimum_required(VERSION 3.0)
project(xcdat VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()

if ((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
    set(CMAKE_COMPILER_IS_CLANGXX 1)
endif ()
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_COMPILER_IS_GNUCXX 1)
endif ()

# C++17 compiler check
if ((CMAKE_COMPILER_IS_GNUCXX AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 7.0) OR (CMAKE_COMPILER_IS_CLANGXX AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 4.0))
    message(FATAL_ERROR "Your C++ compiler does not support C++17. Please install g++ 7.0 (or greater) or clang 4.0 (or greater)")
else ()
    message(STATUS "Compiler is recent enough to support C++17.")
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -pthread -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -march=native -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer -O0 -g -DDEBUG")

message(STATUS "BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
message(STATUS "CXX_FLAGS are ${CMAKE_CXX_FLAGS}")
message(STATUS "CXX_FLAGS_DEBUG are ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CXX_FLAGS_RELEASE are ${CMAKE_CXX_FLAGS_RELEASE}")

include_directories(include)

enable_testing()
add_subdirectory(test)
