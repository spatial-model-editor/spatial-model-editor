target_include_directories(
  core PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
              $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>)
target_include_directories(core PRIVATE src ${CMAKE_SOURCE_DIR}/ext/duneConfig)
if(BUILD_TESTING)
  target_include_directories(
    core_tests
    PUBLIC src
           inc
           ${CMAKE_SOURCE_DIR}/ext/duneConfig)
endif()
add_subdirectory(src)
