
add_executable(xcdat-exe xcdat.cpp)
set_target_properties(xcdat-exe PROPERTIES OUTPUT_NAME xcdat)
target_link_libraries(xcdat-exe xcdat)

install(TARGETS xcdat-exe RUNTIME DESTINATION bin)