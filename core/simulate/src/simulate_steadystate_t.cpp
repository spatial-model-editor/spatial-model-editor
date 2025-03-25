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
        m, simulate::SimulatorType::Pixel, stop_tolerance,
        simulate::SteadystateConvergenceMode::relative, 1000, 1e-2);
    REQUIRE(steadyStateSim.getStopTolerance() == stop_tolerance);
    REQUIRE(steadyStateSim.getSimulatorType() ==
            simulate::SimulatorType::Pixel);
    REQUIRE(steadyStateSim.getDt() == 1e-2);
    REQUIRE(steadyStateSim.getStepsBelowTolerance() == 0);
  }
  SECTION("timeout_simulation") {
    // simulate::PixelSimSteadyState pixelSim(m, comps, specs, stop_tolerance);
    // std::size_t steps = pixelSim.run(10000000, 50, []() { return false; });
    // REQUIRE(pixelSim.getCurrentError() > stop_tolerance);
    // REQUIRE(steps > 0);
    // REQUIRE(pixelSim.errorMessage() == "Simulation stopped early");
  }
  SECTION("Run_until_convergence") {
    // simulate::PixelSimSteadyState pixelSim(m, comps, specs, stop_tolerance);
    // std::size_t steps = pixelSim.run(1000000, 500000, []() { return false;
    // }); if (not pixelSim.errorMessage().empty()) {
    //   SPDLOG_CRITICAL("error message: {}", pixelSim.errorMessage());
    // }
    // REQUIRE(pixelSim.errorMessage().empty());
    // REQUIRE(steps > 0);
    // REQUIRE(pixelSim.getCurrentError() < stop_tolerance);
  }
  SECTION("Callback_is_provided_and_used_to_stop_simulation") {
    // simulate::PixelSimSteadyState pixelSim(m, comps, specs, stop_tolerance);
    // pixelSim.run(1000000, -1, []() { return true; });
    // REQUIRE(pixelSim.errorMessage() == "Simulation stopped early");
  }
}
