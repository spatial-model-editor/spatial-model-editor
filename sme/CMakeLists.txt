pybind11_add_module(sme MODULE)

if(SME_QT_DISABLE_UNICODE)
  qt6_disable_unicode_defines(sme)
endif()

target_link_libraries(sme PRIVATE sme::core ${SME_EXTRA_EXE_LIBS})
target_sources(sme PRIVATE sme.cpp)
target_include_directories(sme PRIVATE src)
add_subdirectory(src)

set_target_properties(sme PROPERTIES CXX_STANDARD 20)
set_target_properties(sme PROPERTIES POSITION_INDEPENDENT_CODE ON)

# https://github.com/pybind/pybind11/issues/1818#issuecomment-506031452
target_compile_options(sme
                       PUBLIC $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>)

install(TARGETS sme DESTINATION .)

qt_import_plugins(
  sme
  EXCLUDE_BY_TYPE
  platforms
  EXCLUDE_BY_TYPE
  platformthemes
  EXCLUDE_BY_TYPE
  printsupport
  EXCLUDE_BY_TYPE
  sqldrivers)
