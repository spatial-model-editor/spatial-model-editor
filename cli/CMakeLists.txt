add_library(cli STATIC)
target_include_directories(cli PUBLIC inc)
target_include_directories(cli PUBLIC src)
target_link_libraries(
  cli
  PUBLIC CLI11::CLI11
         core
         ${SME_EXTRA_EXE_LIBS})

add_library(cli_tests STATIC)
target_include_directories(cli_tests PUBLIC inc src)
target_link_libraries(
  cli_tests
  PUBLIC CLI11::CLI11
         cli
         testlib)

add_subdirectory(src)

add_executable(spatial-cli spatial-cli.cpp)
target_link_libraries(spatial-cli PRIVATE cli)
qt5_import_plugins(
  spatial-cli
  EXCLUDE_BY_TYPE
  platforms
  EXCLUDE_BY_TYPE
  platformthemes
  EXCLUDE_BY_TYPE
  printsupport
  EXCLUDE_BY_TYPE
  sqldrivers)
