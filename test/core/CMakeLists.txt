add_subdirectory(mesh)

target_sources(tests
               PRIVATE
               test_dune.cpp
               test_geometry.cpp
               test_pde.cpp
               test_sbml.cpp
               test_simulate.cpp
               test_symbolic.cpp
               test_units.cpp
               test_utils.cpp)

