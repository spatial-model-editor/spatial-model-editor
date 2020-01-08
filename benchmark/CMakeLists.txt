add_executable(benchmark benchmark.cpp)
target_compile_features(benchmark PRIVATE cxx_std_17)
target_link_libraries(benchmark PRIVATE core)
