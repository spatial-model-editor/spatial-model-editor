add_executable(benchmark benchmark.cpp)
target_compile_features(benchmark PRIVATE cxx_std_17)
target_link_libraries(benchmark PRIVATE ${SME_EXTRA_EXE_LIBS} core)
qt5_import_plugins(
  benchmark
  EXCLUDE_BY_TYPE
  platforms
  EXCLUDE_BY_TYPE
  platformthemes
  EXCLUDE_BY_TYPE
  printsupport
  EXCLUDE_BY_TYPE
  sqldrivers)

add_executable(pixel pixel.cpp)
target_compile_features(pixel PRIVATE cxx_std_17)
target_link_libraries(pixel PRIVATE ${SME_EXTRA_EXE_LIBS} core)
qt5_import_plugins(
  pixel
  EXCLUDE_BY_TYPE
  platforms
  EXCLUDE_BY_TYPE
  platformthemes
  EXCLUDE_BY_TYPE
  printsupport
  EXCLUDE_BY_TYPE
  sqldrivers)

find_package(benchmark REQUIRED)
add_executable(bench bench.cpp)
target_link_libraries(
  bench
  PUBLIC ${SME_EXTRA_EXE_LIBS}
         core_tests
         benchmark::benchmark)
