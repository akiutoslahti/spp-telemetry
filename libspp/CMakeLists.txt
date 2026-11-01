add_library(spp STATIC src/primary_header.cpp)
target_link_libraries(spp PRIVATE project-warnings)
target_compile_features(spp PUBLIC cxx_std_20)
target_include_directories(spp PUBLIC include)

if (BUILD_TESTING)
  add_subdirectory(tests)
endif()
