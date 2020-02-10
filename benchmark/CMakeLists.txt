add_executable(benchmark benchmark.cpp)
target_compile_features(benchmark PRIVATE cxx_std_17)
target_link_libraries(benchmark PRIVATE core)

add_executable(pixel pixel.cpp)
target_compile_features(pixel PRIVATE cxx_std_17)
target_link_libraries(pixel PRIVATE core)
