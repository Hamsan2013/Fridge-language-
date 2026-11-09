add_executable(fridge_tests test_runner.cpp)
target_link_libraries(fridge_tests PRIVATE fridge_core)
add_test(NAME ComprehensiveSuite COMMAND fridge_tests)
