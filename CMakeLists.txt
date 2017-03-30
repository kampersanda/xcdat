cmake_minimum_required(VERSION 2.8)
project(XCDAT)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -std=c++11")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT XCDAT64)
  set(XCDAT64 OFF)
endif()

if(XCDAT64)
  add_definitions(-DXCDAT64)
endif()

message(STATUS "BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
message(STATUS "XCDAT64 is ${XCDAT64}")

enable_testing()
add_subdirectory(src)
