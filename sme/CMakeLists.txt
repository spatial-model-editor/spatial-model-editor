pybind11_add_module(
  sme
  SYSTEM
  sme.cpp)

target_sources(
  sme
  PRIVATE sme_compartment.cpp
          sme_model.cpp
          sme_module.cpp
          sme_reaction.cpp
          sme_reactionparameter.cpp
          sme_species.cpp)

target_compile_features(sme PRIVATE cxx_std_17)

target_link_libraries(sme PRIVATE ${SME_EXTRA_EXE_LIBS} core)

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
