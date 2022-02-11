target_include_directories(
  core PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
              $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)
if(BUILD_TESTING)
  target_include_directories(core_tests PUBLIC src include)
endif()
add_subdirectory(src)
