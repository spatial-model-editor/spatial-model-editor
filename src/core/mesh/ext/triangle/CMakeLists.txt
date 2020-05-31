add_library(triangle STATIC)
target_include_directories(triangle PUBLIC inc)
add_subdirectory(src)
set_property(TARGET triangle PROPERTY POSITION_INDEPENDENT_CODE ON)
