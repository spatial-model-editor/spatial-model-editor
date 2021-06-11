add_library(cli STATIC)
target_include_directories(cli PUBLIC src)
target_link_libraries(
  cli
  PUBLIC CLI11::CLI11
         sme::core
         ${SME_EXTRA_EXE_LIBS})

if(BUILD_TESTING)
  add_library(cli_tests STATIC)
  target_include_directories(cli_tests PUBLIC inc src)
  target_link_libraries(
    cli_tests
    PUBLIC CLI11::CLI11
           cli
           testlib)
endif()

if(SME_QT_DISABLE_UNICODE)
  qt6_disable_unicode_defines(cli)
  if(BUILD_TESTING)
    qt6_disable_unicode_defines(cli_tests)
  endif()
endif()

add_subdirectory(src)

add_executable(spatial-cli spatial-cli.cpp)
if(SME_QT_DISABLE_UNICODE)
  qt6_disable_unicode_defines(spatial-cli)
endif()
target_link_libraries(spatial-cli PRIVATE cli)
qt_import_plugins(
  spatial-cli
  EXCLUDE_BY_TYPE
  platforms
  EXCLUDE_BY_TYPE
  platformthemes
  EXCLUDE_BY_TYPE
  printsupport
  EXCLUDE_BY_TYPE
  sqldrivers)
