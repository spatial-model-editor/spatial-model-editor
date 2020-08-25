target_sources(
  core
  PRIVATE basesim.cpp
          duneconverter.cpp
          dunefunction.cpp
          dunegrid.cpp
          duneini.cpp
          dunesim.cpp
          dunesim_impl.cpp
          dunesim_impl_coupled.cpp
          dunesim_impl_independent.cpp
          pde.cpp
          pixelsim.cpp
          pixelsim_impl.cpp
          simulate.cpp)

target_sources(
  core_tests
  PUBLIC duneconverter_t.cpp
         dunefunction_t.cpp
         dunegrid_t.cpp
         duneini_t.cpp
         pde_t.cpp
         simulate_t.cpp)
