target_include_directories(core PUBLIC inc)
target_include_directories(core PRIVATE src ${CMAKE_SOURCE_DIR}/ext/duneConfig)
if(BUILD_TESTING)
  target_include_directories(
    core_tests
    PUBLIC src
           inc
           ${CMAKE_SOURCE_DIR}/ext/duneConfig)
endif()
add_subdirectory(src)
