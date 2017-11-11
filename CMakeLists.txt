cmake_minimum_required(VERSION 2.8)
project(XCDAT)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()

configure_file(
  ${XCDAT_SOURCE_DIR}/xcdat_config.hpp.in
  ${XCDAT_SOURCE_DIR}/src/xcdat_config.hpp
)

option(XCDAT_X64
  "Use 64-bit integers to represent nodes."
  OFF)

option(XCDAT_USE_POPCNT
  "Use popcount intrinsic. Available on x86-64 since SSE4.2."
  OFF)

if (XCDAT_USE_POPCNT)
  if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
  endif ()
endif ()

message(STATUS "BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
message(STATUS "CXX_FLAGS are ${CMAKE_CXX_FLAGS}")
message(STATUS "CXX_FLAGS_DEBUG are ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CXX_FLAGS_RELEASE are ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "XCDAT_X64 is ${XCDAT_X64}")
message(STATUS "XCDAT_USE_POPCNT is ${XCDAT_USE_POPCNT}")

set(HEADER_FILES
  src/BitVector.hpp
  src/BitVectorBuilder.hpp
  src/DacBc.hpp
  src/FastDacBc.hpp
  src/FitVector.hpp
  src/Trie.hpp
  src/TrieBuilder.hpp
  src/Vector.hpp
  src/xcdat_basics.hpp
  src/xcdat_config.hpp
  src/xcdat.hpp)

set(SOURCE_FILES
  src/BitVector.cpp
  src/DacBc.cpp
  src/FitVector.cpp
  src/TrieBuilder.cpp
  src/FastDacBc.cpp
  )

add_library(xcdat STATIC ${HEADER_FILES} ${SOURCE_FILES})

add_executable(xcdat-exe src/xcdat.cpp)
set_target_properties(xcdat-exe PROPERTIES OUTPUT_NAME xcdat)
target_link_libraries(xcdat-exe xcdat)

enable_testing()
file(GLOB TEST_SOURCES src/*_test.cpp)
foreach(TEST_SOURCE ${TEST_SOURCES})
  get_filename_component(TEST_SOURCE_NAME ${TEST_SOURCE} NAME_WE)
  add_executable(${TEST_SOURCE_NAME} ${TEST_SOURCE})
  target_link_libraries(${TEST_SOURCE_NAME} xcdat)
  add_test(${TEST_SOURCE_NAME} ${TEST_SOURCE_NAME})
endforeach()

INSTALL(FILES ${HEADER_FILES} DESTINATION include/xcdat)
INSTALL(TARGETS xcdat ARCHIVE DESTINATION lib)
INSTALL(TARGETS xcdat-exe RUNTIME DESTINATION bin)