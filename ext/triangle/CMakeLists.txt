add_library(triangle STATIC triangle.cpp)
target_include_directories(triangle INTERFACE ${PROJECT_SOURCE_DIR}/ext)
set_property(TARGET triangle PROPERTY POSITION_INDEPENDENT_CODE ON)
