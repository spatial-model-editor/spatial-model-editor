target_sources(cli PRIVATE cli_params.cpp cli_simulate.cpp)

if(BUILD_TESTING)
  target_sources(cli_tests PUBLIC cli_params_t.cpp cli_simulate_t.cpp)
endif()
