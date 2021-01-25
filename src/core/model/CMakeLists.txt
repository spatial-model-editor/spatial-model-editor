target_include_directories(
  core PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
              $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(core PRIVATE src)
if(BUILD_TESTING)
  target_include_directories(core_tests PUBLIC inc src)
endif()
if(BUILD_BENCHMARKS)
  target_include_directories(bench PUBLIC inc src)
endif()
add_subdirectory(src)
