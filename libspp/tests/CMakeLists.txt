add_executable(libspp-tests dummy.cpp)
target_link_libraries(libspp-tests PRIVATE Catch2::Catch2WithMain spp project-warnings)
catch_discover_tests(libspp-tests)
