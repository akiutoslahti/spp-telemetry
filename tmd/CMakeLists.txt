add_library(tmd-core INTERFACE)
target_compile_features(tmd-core INTERFACE cxx_std_20)
target_include_directories(tmd-core INTERFACE include)

if (BUILD_TESTING)
  add_subdirectory(tests)
endif()
