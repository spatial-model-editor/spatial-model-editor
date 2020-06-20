target_include_directories(core PUBLIC inc)
target_include_directories(core PRIVATE src ${CMAKE_SOURCE_DIR}/ext/duneConfig)
target_include_directories(core_tests PUBLIC src inc)
add_subdirectory(src)
