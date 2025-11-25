# Root tests CMake which groups tests into unit/integration categories
cmake_minimum_required(VERSION 3.10)

# Integration tests
add_executable(test_media_basic integration/media_tests_basic.cpp)
add_dependencies(test_media_basic IM)
target_link_libraries(test_media_basic PRIVATE IM)

add_executable(test_media_edge integration/media_tests_edge_cases.cpp)
add_dependencies(test_media_edge IM)
target_link_libraries(test_media_edge PRIVATE IM)

add_executable(test_media_concurrency integration/media_tests_concurrency.cpp)
add_dependencies(test_media_concurrency IM)
target_link_libraries(test_media_concurrency PRIVATE IM)

add_executable(test_media_idempotency integration/media_tests_idempotency.cpp)
add_dependencies(test_media_idempotency IM)
target_link_libraries(test_media_idempotency PRIVATE IM)

# Set runtime output directory if parent set BIN_TEST_DIR
if(BIN_TEST_DIR)
    set_target_properties(test_media_basic PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_TEST_DIR})
    set_target_properties(test_media_edge PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_TEST_DIR})
    set_target_properties(test_media_concurrency PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_TEST_DIR})
    set_target_properties(test_media_idempotency PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_TEST_DIR})
endif()
