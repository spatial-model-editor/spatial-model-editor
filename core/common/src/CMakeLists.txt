configure_file("version.cpp.in"
               "${PROJECT_BINARY_DIR}/src/core/common/src/version.cpp")
target_sources(
  core
  PRIVATE image_stack.cpp
          logger.cpp
          serialization.cpp
          simple_symbolic.cpp
          symbolic.cpp
          tiff.cpp
          utils.cpp
          ${PROJECT_BINARY_DIR}/src/core/common/src/version.cpp)
if(BUILD_TESTING)
  target_sources(
    core_tests
    PUBLIC image_stack_t.cpp
           logger_t.cpp
           serialization_t.cpp
           simple_symbolic_t.cpp
           symbolic_t.cpp
           tiff_t.cpp
           utils_t.cpp
           voxel_t.cpp)
endif()
