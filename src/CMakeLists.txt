set(HEADER_FILES
  BitVector.hpp
  BitVectorBuilder.hpp
  CharRange.hpp
  DacBc.hpp
  Exception.hpp
  FastDacBc.hpp
  SmallVector.hpp
  Trie.hpp
  TrieBuilder.hpp
  xcdatBasics.hpp
  )

set(SOURCE_FILES
  ${HEADER_FILES}
  BitVector.cpp
  DacBc.cpp
  SmallVector.cpp
  TrieBuilder.cpp
  FastDacBc.cpp
  )

add_library(xcdat STATIC ${SOURCE_FILES})

add_executable(xcdat-exe xcdat.cpp)
set_target_properties(xcdat-exe PROPERTIES OUTPUT_NAME xcdat)
target_link_libraries(xcdat-exe xcdat)

enable_testing()
file(GLOB TEST_SOURCES test*.cpp)
foreach(TEST_SOURCE ${TEST_SOURCES})
  get_filename_component(TEST_SOURCE_NAME ${TEST_SOURCE} NAME_WE)
  add_executable(${TEST_SOURCE_NAME} ${TEST_SOURCE})
  target_link_libraries(${TEST_SOURCE_NAME} xcdat)
  add_test(${TEST_SOURCE_NAME} ${TEST_SOURCE_NAME})
endforeach()
