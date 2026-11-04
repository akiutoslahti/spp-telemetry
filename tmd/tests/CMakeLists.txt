add_executable(tmd-tests test_uniquefd.cpp test_statistics.cpp)
target_link_libraries(tmd-tests PRIVATE Catch2::Catch2WithMain tmd-core project-warnings)
catch_discover_tests(tmd-tests)
