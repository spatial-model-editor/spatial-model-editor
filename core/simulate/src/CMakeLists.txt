target_sources(
  core
  PRIVATE basesim.cpp
          dune_cell_data.cpp
          feature_eval.cpp
          feature_options.cpp
          duneconverter.cpp
          duneconverter_impl.cpp
          dunefunction.cpp
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

if(SME_WITH_CUDA)
  target_sources(core PRIVATE cuda_stubs.cpp cudapixelsim.cpp)
endif()
if(SME_WITH_METAL)
  target_sources(core PRIVATE metalpixelsim.cpp)
endif()

if(BUILD_TESTING)
  target_sources(
    core_tests
    PUBLIC feature_eval_t.cpp
           feature_options_t.cpp
           duneconverter_t.cpp
           duneconverter_impl_t.cpp
           dune_cell_data_t.cpp
           dunefunction_t.cpp
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
  if(SME_WITH_CUDA OR SME_WITH_METAL)
    target_sources(core_tests PUBLIC cudapixelsim_t.cpp)
  endif()
endif()
if(SME_BUILD_BENCHMARKS)
  target_sources(bench PUBLIC duneconverter_bench.cpp simulate_bench.cpp)
endif()
