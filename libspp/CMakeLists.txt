add_library(spp INTERFACE)
target_compile_features(spp INTERFACE cxx_std_20)
target_include_directories(spp INTERFACE include)

if (BUILD_TESTING)
  add_subdirectory(tests)
endif()
