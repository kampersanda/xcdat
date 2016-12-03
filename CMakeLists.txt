cmake_minimum_required(VERSION 2.8)
project(XCDAT)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "BUILD_TYPE is ${CMAKE_BUILD_TYPE}")

enable_testing()

add_subdirectory(src)
