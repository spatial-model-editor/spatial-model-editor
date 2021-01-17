add_executable(benchmark benchmark.cpp)
target_compile_features(benchmark PRIVATE cxx_std_17)
target_link_libraries(benchmark PRIVATE core ${SME_EXTRA_EXE_LIBS})

add_executable(pixel pixel.cpp)
target_compile_features(pixel PRIVATE cxx_std_17)
target_link_libraries(pixel PRIVATE core ${SME_EXTRA_EXE_LIBS})

find_package(benchmark REQUIRED)
add_executable(bench bench.cpp)
target_include_directories(bench PUBLIC .)
target_link_libraries(bench PUBLIC benchmark::benchmark ${SME_EXTRA_EXE_LIBS})
