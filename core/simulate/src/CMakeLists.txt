target_sources(
  core
  PRIVATE basesim.cpp
          duneconverter.cpp
          duneconverter_impl.cpp
          dunefunction.cpp
          dunefunctorfactory.cpp
          dunegrid.cpp
          duneini.cpp
          dunesim.cpp
          dunesim_impl.cpp
          optimize.cpp
          optimize_impl.cpp
          optimize_options.cpp
          pde.cpp
          pixelsim.cpp
          pixelsim_impl.cpp
          simulate_steadystate.cpp
          simulate.cpp
          simulate_data.cpp
          simulate_options.cpp)

if(BUILD_TESTING)
  target_sources(
    core_tests
    PUBLIC duneconverter_t.cpp
           duneconverter_impl_t.cpp
           dunefunction_t.cpp
           dunefunctorfactory_t.cpp
           dunegrid_t.cpp
           duneini_t.cpp
           dunesim_t.cpp
           optimize_t.cpp
           optimize_impl_t.cpp
           optimize_options_t.cpp
           pde_t.cpp
           pixelsim_t.cpp
           simulate_data_t.cpp
           simulate_options_t.cpp
           simulate_t.cpp
           simulate_steadystate_t.cpp)
endif()
if(SME_BUILD_BENCHMARKS)
  target_sources(bench PUBLIC duneconverter_bench.cpp simulate_bench.cpp)
endif()
