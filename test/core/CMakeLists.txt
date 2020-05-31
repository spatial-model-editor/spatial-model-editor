add_subdirectory(mesh)
add_subdirectory(sbml)
add_subdirectory(simulate)

target_sources(tests
               PRIVATE
               test_dune.cpp
               test_geometry.cpp
               test_pde.cpp
               test_symbolic.cpp
               test_units.cpp
               test_utils.cpp)

