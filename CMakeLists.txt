cmake_minimum_required(VERSION 3.1)
project(XCDAT)

set(CMAKE_CXX_STANDARD 17)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()

configure_file(
  ${XCDAT_SOURCE_DIR}/xcdat_config.hpp.in
  ${XCDAT_SOURCE_DIR}/include/xcdat/xcdat_config.hpp
)

message(STATUS "XCDAT_SOURCE_DIR is ${XCDAT_SOURCE_DIR}")

option(XCDAT_X64
  "Use 64-bit integers for node representation."
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

file(GLOB HEADER_FILES include/xcdat/*.hpp)
file(GLOB SOURCE_FILES lib/*.cpp)

include_directories(include)
add_library(xcdat STATIC ${HEADER_FILES} ${SOURCE_FILES})

add_subdirectory(tool)
add_subdirectory(sample)

enable_testing()
add_subdirectory(test)

install(FILES include/xcdat.hpp DESTINATION include)
install(FILES ${HEADER_FILES} DESTINATION include/xcdat)

install(TARGETS xcdat
  EXPORT xcdat-targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin)

install(EXPORT xcdat-targets
  FILE xcdat-config.cmake
  DESTINATION lib/cmake/xcdat)
