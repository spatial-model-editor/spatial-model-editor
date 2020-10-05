if(WIN32)
  # don't use pybind11_add_module() on windows due to issues with flto and
  # mingw32
  add_library(sme MODULE)
  target_link_libraries(sme PRIVATE pybind11::module)
  set_target_properties(sme PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}"
                                       SUFFIX "${PYTHON_MODULE_EXTENSION}")
else()
  pybind11_add_module(sme SYSTEM)
endif()

target_link_libraries(sme PRIVATE core ${SME_EXTRA_EXE_LIBS})

target_sources(
  sme
  PRIVATE sme.cpp
          sme_common.cpp
          sme_compartment.cpp
          sme_membrane.cpp
          sme_model.cpp
          sme_module.cpp
          sme_parameter.cpp
          sme_reaction.cpp
          sme_reactionparameter.cpp
          sme_simulationresult.cpp
          sme_species.cpp)

set_target_properties(sme PROPERTIES CXX_STANDARD 17)
set_target_properties(sme PROPERTIES POSITION_INDEPENDENT_CODE ON)

# https://github.com/pybind/pybind11/issues/1818#issuecomment-506031452
target_compile_options(sme
                       PUBLIC $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>)

# including python 2.7 headers gives -Wregister warnings with gcc (and errors
# with clang)
target_compile_options(sme PUBLIC $<$<CXX_COMPILER_ID:GNU>:-Wno-register>)

qt5_import_plugins(
  sme
  EXCLUDE_BY_TYPE
  platforms
  EXCLUDE_BY_TYPE
  platformthemes
  EXCLUDE_BY_TYPE
  printsupport
  EXCLUDE_BY_TYPE
  sqldrivers)
