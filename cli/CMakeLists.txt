add_executable(spatial-cli spatial-cli.cpp)
target_compile_features(spatial-cli PRIVATE cxx_std_17)
target_link_libraries(spatial-cli PRIVATE core)
