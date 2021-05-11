if(WIN32)
  # don't use pybind11_add_module() on windows due to issues with flto and
  # mingw32
  add_library(sme MODULE)
  target_link_libraries(sme PRIVATE pybind11::module)
  set_target_properties(sme PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}"
                                       SUFFIX "${PYTHON_MODULE_EXTENSION}")
else()
  pybind11_add_module(sme MODULE)
endif()

target_link_libraries(sme PRIVATE sme::core ${SME_EXTRA_EXE_LIBS})
target_sources(sme PRIVATE sme.cpp)
target_include_directories(sme PRIVATE src)
add_subdirectory(src)

set_target_properties(sme PROPERTIES CXX_STANDARD 17)
set_target_properties(sme PROPERTIES POSITION_INDEPENDENT_CODE ON)

# https://github.com/pybind/pybind11/issues/1818#issuecomment-506031452
target_compile_options(sme
                       PUBLIC $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>)

# including python 2.7 headers gives -Wregister warnings with gcc (and errors
# with clang)
target_compile_options(sme PUBLIC $<$<CXX_COMPILER_ID:GNU>:-Wno-register>)

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
