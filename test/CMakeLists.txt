
file(GLOB TEST_SOURCES *_test.cpp)
foreach(TEST_SOURCE ${TEST_SOURCES})
  get_filename_component(TEST_SOURCE_NAME ${TEST_SOURCE} NAME_WE)
  add_executable(${TEST_SOURCE_NAME} ${TEST_SOURCE})
  target_link_libraries(${TEST_SOURCE_NAME} xcdat)
  add_test(${TEST_SOURCE_NAME} ${TEST_SOURCE_NAME})
endforeach()
