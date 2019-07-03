
add_executable(
    logging_test logging_test.cpp
)


target_link_libraries(
    logging_test gtest_main
)

add_executable(
    program_options_test program_options_test.cpp
)

target_link_libraries(program_options_test gtest_main)

add_executable(client_test client_test.cpp)
target_link_libraries(client_test gtest_main boost_system ssl crypto)
