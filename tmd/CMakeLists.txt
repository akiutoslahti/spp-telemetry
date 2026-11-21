add_library(tmd-core STATIC src/descriptors.cpp src/eventloop.cpp src/control.cpp src/error.cpp)
target_link_libraries(tmd-core PUBLIC spp)
target_link_libraries(tmd-core PRIVATE project-warnings)
target_compile_features(tmd-core PUBLIC cxx_std_20)
target_include_directories(tmd-core PUBLIC include)

if (BUILD_TESTING)
  add_subdirectory(tests)
endif()
