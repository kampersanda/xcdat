add_executable(test_bit_vector test_bit_vector.cpp)
add_test(test_bit_vector test_bit_vector)

add_executable(test_compact_vector test_compact_vector.cpp)
add_test(test_compact_vector test_compact_vector)

add_executable(test_tail_vector test_tail_vector.cpp)
add_test(test_tail_vector test_tail_vector)

set(BC_OPTIONS "7" "8")

foreach(BC_OPTION ${BC_OPTIONS})
    set(TEST_SRC_NAME test_bc_vector_${BC_OPTION})
    add_executable(${TEST_SRC_NAME} test_bc_vector.cpp)
    set_target_properties(${TEST_SRC_NAME} PROPERTIES COMPILE_DEFINITIONS BC_VECTOR_${BC_OPTION})
    add_test(${TEST_SRC_NAME} ${TEST_SRC_NAME})
endforeach(BC_OPTION)

foreach(BC_OPTION ${BC_OPTIONS})
    set(TEST_SRC_NAME test_trie_${BC_OPTION})
    add_executable(${TEST_SRC_NAME} test_trie.cpp)
    set_target_properties(${TEST_SRC_NAME} PROPERTIES COMPILE_DEFINITIONS TRIE_${BC_OPTION})
    add_test(${TEST_SRC_NAME} ${TEST_SRC_NAME})
endforeach(BC_OPTION)
