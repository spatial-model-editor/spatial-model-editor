add_subdirectory(ext/triangle)
target_include_directories(
  core PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
              $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(core PRIVATE src)
target_link_libraries(core PRIVATE triangle)
if(BUILD_TESTING)
  target_include_directories(core_tests PUBLIC inc src)
  target_link_libraries(core_tests PUBLIC triangle)
endif()
add_subdirectory(src)
