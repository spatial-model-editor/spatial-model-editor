#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "simulate_steadystate.hpp"
#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <vector>

using namespace sme;
using namespace sme::test;

TEST_CASE("SimulateSteadyState",
          "[core/simulate/simulate_steadystate][core/"
          "simulate][core][simulate][simulate_steadystate]") {
  auto m{getExampleModel(Mod::GrayScott)};
  std::vector<std::string> comps{"compartment"};
  std::vector<std::vector<std::string>> specs{{"U", "V"}};
  double stop_tolerance = 1e-8;
  SECTION("construct") {
    simulate::SteadyStateSimulation steadyStateSim(
        m, simulate::SimulatorType::Pixel, stop_tolerance, 10,
        simulate::SteadystateConvergenceMode::relative, 1000, 1e-2);
    REQUIRE(steadyStateSim.getStopTolerance() == stop_tolerance);
    REQUIRE(steadyStateSim.getSimulatorType() ==
            simulate::SimulatorType::Pixel);
    REQUIRE(steadyStateSim.getDt() == 1e-2);
    REQUIRE(steadyStateSim.getStepsBelowTolerance() == 0);

    steadyStateSim.setStopTolerance(1e-9);
    REQUIRE(steadyStateSim.getStopTolerance() == 1e-9);

    steadyStateSim.setSimulatorType(simulate::SimulatorType::DUNE);
    REQUIRE(steadyStateSim.getSimulatorType() == simulate::SimulatorType::DUNE);

    steadyStateSim.setStepsBelowTolerance(5);
    REQUIRE(steadyStateSim.getStepsBelowTolerance() == 5);

    steadyStateSim.setStopMode(simulate::SteadystateConvergenceMode::absolute);
    REQUIRE(steadyStateSim.getConvergenceMode() ==
            simulate::SteadystateConvergenceMode::absolute);
  }
  SECTION("Run_until_convergence_pixel") {
    simulate::SteadyStateSimulation steadyStateSim(
        m, simulate::SimulatorType::Pixel, stop_tolerance, 10,
        simulate::SteadystateConvergenceMode::relative, 1000, 1e-2);
  }
  SECTION("Run_until_convergence_dune") {
    simulate::SteadyStateSimulation steadyStateSim(
        m, simulate::SimulatorType::Pixel, stop_tolerance, 10,
        simulate::SteadystateConvergenceMode::relative, 1000, 1e-2);
  }
  SECTION("timeout_simulation") {
    simulate::SteadyStateSimulation steadyStateSim(
        m, simulate::SimulatorType::Pixel, stop_tolerance, 10,
        simulate::SteadystateConvergenceMode::relative, 1000, 1e-2);
  }
}
