#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "sme/simulate_options.hpp"
#include "sme/simulate_steadystate.hpp"
#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <limits>
#include <spdlog/spdlog.h>
#include <vector>

using namespace sme;
using namespace sme::test;

TEST_CASE("SimulateSteadyState",
          "[core/simulate/simulate_steadystate][core/"
          "simulate][core][simulate][simulate_steadystate]") {
  auto m{getExampleModel(Mod::GrayScott)};
  std::vector<std::string> comps{"compartment"};
  std::vector<std::vector<std::string>> specs{{"U", "V"}};
  double stop_tolerance = 1e-6;

  SECTION("construct") {
    simulate::SteadyStateSimulation sim(
        m, simulate::SimulatorType::Pixel, stop_tolerance, 10,
        simulate::SteadystateConvergenceMode::relative, 1000, 1e-2);
    REQUIRE(sim.getStopTolerance() == stop_tolerance);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::Pixel);
    REQUIRE(sim.getDt() == 1e-2);
    REQUIRE(sim.getStepsBelowTolerance() == 0);

    sim.setStopTolerance(1e-9);
    REQUIRE(sim.getStopTolerance() == 1e-9);

    sim.setSimulatorType(simulate::SimulatorType::DUNE);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::DUNE);

    sim.setStepsBelowTolerance(5);
    REQUIRE(sim.getStepsBelowTolerance() == 5);

    sim.setConvergenceMode(simulate::SteadystateConvergenceMode::absolute);
    REQUIRE(sim.getConvergenceMode() ==
            simulate::SteadystateConvergenceMode::absolute);

    REQUIRE(sim.getSteps().size() == 0);
    REQUIRE(sim.getErrors().size() == 0);
    REQUIRE(sim.getCurrentError() == std::numeric_limits<double>::max());
    REQUIRE(sim.getCurrentStep() == 0);
  }
  SECTION("Run_into_timeout_and_query") {
    simulate::SteadyStateSimulation sim(
        m, simulate::SimulatorType::Pixel, stop_tolerance, 10,
        simulate::SteadystateConvergenceMode::relative, 50, 1e-2);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::Pixel);
    REQUIRE(sim.hasConverged() == false);
    sim.run(); // run for 500 ms then timeout
    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getSteps().size() > 0);
    REQUIRE(sim.getErrors().size() > 0);
    REQUIRE(sim.getCurrentError() > 0.);
    REQUIRE(sim.getCurrentStep() > 0);
    REQUIRE(!sim.hasConverged());
    REQUIRE(sim.getStepsBelowTolerance() == 0);
    REQUIRE(sim.getSolverStopRequested() == true);
    REQUIRE(sim.getSolverErrormessage() == "Simulation timed out");
  }
  SECTION("Run_until_convergence_pixel") {
    simulate::SteadyStateSimulation sim(
        m, simulate::SimulatorType::Pixel, stop_tolerance, 10,
        simulate::SteadystateConvergenceMode::relative, 100000000, 5.0);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::Pixel);
    REQUIRE(sim.hasConverged() == false);
    sim.run(); // run until convergence
    REQUIRE(sim.hasConverged());
    REQUIRE(sim.getSolverStopRequested());
    REQUIRE(sim.getCurrentError() < stop_tolerance);
    REQUIRE(sim.getStepsBelowTolerance() == sim.getStepsToConvergence());
    REQUIRE(sim.getSolverErrormessage() == "");
  }
  SECTION("Run_until_convergence_dune") {

    // we know that it converges, hence timeout is absurd here and we set
    // a large timestep to check for convergence too.
    simulate::SteadyStateSimulation sim(
        m, simulate::SimulatorType::DUNE, stop_tolerance, 10,
        simulate::SteadystateConvergenceMode::relative, 100000000, 5.0);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::DUNE);
    REQUIRE(sim.hasConverged() == false);
    sim.run(); // run until convergence
    REQUIRE(sim.hasConverged() == true);
    REQUIRE(sim.getCurrentError() < stop_tolerance);
    REQUIRE(sim.getStepsBelowTolerance() == sim.getStepsToConvergence());
    REQUIRE(sim.getSolverErrormessage() == "");
  }
  SECTION("change_solver-->simulation_reset") {
    simulate::SteadyStateSimulation sim(
        m, simulate::SimulatorType::Pixel, stop_tolerance, 10,
        simulate::SteadystateConvergenceMode::relative, 10000000, 5.0);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::Pixel);
    sim.run(); // run until convergence
    REQUIRE(sim.hasConverged());
    REQUIRE(sim.getSolverStopRequested());
    REQUIRE(sim.getCurrentError() < stop_tolerance);
    REQUIRE(sim.getStepsBelowTolerance() == sim.getStepsToConvergence());

    sim.setSimulatorType(
        simulate::SimulatorType::DUNE); // reset happens when solver is changed

    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::DUNE);
    REQUIRE(sim.getSteps().size() == 0);
    REQUIRE(sim.getErrors().size() == 0);
    REQUIRE(sim.getCurrentError() == std::numeric_limits<double>::max());
    REQUIRE(sim.getCurrentStep() == 0);
    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getSolverStopRequested() == false);

    sim.setTimeout(500); // set this sucht that it breaks early to avoid runtime
    sim.run();           // run until convergence
    REQUIRE(sim.getErrors().size() > 0);
    REQUIRE(sim.getSteps().size() > 0);
  }
}
