target_sources(
  core
  PRIVATE basesim.hpp
          duneini.cpp
          dunesim.cpp
          dunesim.hpp
          pde.cpp
          pixelsim.cpp
          pixelsim.hpp
          simulate.cpp)

target_sources(
  core_tests
  PUBLIC duneini_t.cpp
         pde_t.cpp
         simulate_t.cpp)
