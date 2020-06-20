add_executable(spatial-cli spatial-cli.cpp)
target_compile_features(spatial-cli PRIVATE cxx_std_17)
target_link_libraries(spatial-cli PRIVATE core ${SME_EXTRA_EXE_LIBS})
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
