target_include_directories(core PUBLIC inc)
target_include_directories(core PRIVATE src)
target_include_directories(core_tests PUBLIC inc src)
add_subdirectory(src)
