# compile tests
add_library(triangle STATIC triangle.cpp)
target_include_directories(triangle INTERFACE ${PROJECT_SOURCE_DIR}/ext)
