#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "sme/simulate_options.hpp"
#include "sme/simulate_steadystate.hpp"
#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <qrgb.h>
#include <spdlog/spdlog.h>
#include <vector>

using namespace sme;
using namespace sme::test;

TEST_CASE(
    "SimulateSteadyState",
    "[core][core/simulate][steadystate][core/simulate/simulate_steadystate]") {

  auto m{getExampleModel(Mod::GrayScott)};
  const std::vector<std::string> comps{"compartment"};
  const std::vector<std::vector<std::string>> specs{{"U", "V"}};
  const double rel_stop_tolerance = 1e-3;
  const double abs_stop_tolerance = 1e-2;

  SECTION("construct") {
    m.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
    simulate::SteadyStateSimulation sim(
        m, rel_stop_tolerance, 10,
        simulate::SteadyStateConvergenceMode::relative, 1000, 1e-2);
    REQUIRE(sim.getStopTolerance() == rel_stop_tolerance);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::Pixel);
    REQUIRE(sim.getDt() == 1e-2);
    REQUIRE(sim.getStepsBelowTolerance() == 0);
    REQUIRE(sim.getConvergenceMode() ==
            simulate::SteadyStateConvergenceMode::relative);
    REQUIRE(sim.getStepsToConvergence() == 10);
    REQUIRE(sim.getTimeout() == 1000);
    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getConcentrations().size() ==
            6298); // number of values for each species in each compartment
                   // concatenated
    REQUIRE(sim.getLatestError() == std::numeric_limits<double>::max());
    REQUIRE(sim.getLatestStep() == 0.0);
    REQUIRE(sim.getSolverErrormessage() == "");
    REQUIRE(sim.getSolverStopRequested() == false);
    REQUIRE(sim.getCompartmentSpeciesIdxs() ==
            std::vector<std::vector<std::size_t>>{{0, 1}});
    auto img = sim.getConcentrationImage(
        std::vector<std::vector<std::size_t>>{{0, 1}}, true);
    REQUIRE(img.empty() == false);
    REQUIRE(img.volume().depth() == 1);
    REQUIRE(img.volume().width() == 100);
    REQUIRE(img.volume().height() == 100);

    sim.setConvergenceMode(simulate::SteadyStateConvergenceMode::absolute);
    REQUIRE(sim.getConvergenceMode() ==
            simulate::SteadyStateConvergenceMode::absolute);

    sim.setStopTolerance(1e-9);
    REQUIRE(sim.getStopTolerance() == 1e-9);

    sim.setStepsToConvergence(20);
    REQUIRE(sim.getStepsToConvergence() == 20);

    sim.setDt(1e-3);
    REQUIRE(sim.getDt() == 1e-3);

    sim.setTimeout(2000);
    REQUIRE(sim.getTimeout() == 2000);
  }

  SECTION("Run_into_timeout_and_query_for_errormessage") {
    m.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
    simulate::SteadyStateSimulation sim(
        m, rel_stop_tolerance, 10,
        simulate::SteadyStateConvergenceMode::relative, 50, 1e-2);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::Pixel);
    REQUIRE(sim.hasConverged() == false);
    sim.run(); // run for 50 ms then timeout
    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getStepsBelowTolerance() < sim.getStepsToConvergence());
    REQUIRE(sim.getSolverStopRequested() == true);
    REQUIRE(sim.getSolverErrormessage() == "Simulation timed out");
  }

  SECTION("Run_until_convergence_pixel") {
    m.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
    simulate::SteadyStateSimulation sim(
        m, rel_stop_tolerance, 10,
        simulate::SteadyStateConvergenceMode::relative, 100000000, 5.0);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::Pixel);
    REQUIRE(sim.hasConverged() == false);
    sim.run(); // run until convergence
    REQUIRE(sim.hasConverged());
    REQUIRE(sim.getSolverStopRequested());
    REQUIRE(sim.getLatestStep() > 0.0);
    REQUIRE(sim.getLatestError() < rel_stop_tolerance);
    REQUIRE(sim.getStepsBelowTolerance() == sim.getStepsToConvergence());
    REQUIRE(sim.getSolverErrormessage() == "");
  }

  SECTION("Run_until_convergence_dune") {
    m.getSimulationSettings().simulatorType = simulate::SimulatorType::DUNE;
    simulate::SteadyStateSimulation sim(
        m, rel_stop_tolerance, 3,
        simulate::SteadyStateConvergenceMode::relative, 100000000, 1.0);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::DUNE);
    REQUIRE(sim.hasConverged() == false);
    sim.run(); // run until convergence
    REQUIRE(sim.hasConverged());
    // stop_requested is ignored by DUNE, hence not checked
    REQUIRE(sim.getLatestStep() > 0.0);
    REQUIRE(sim.getLatestError() < rel_stop_tolerance);
    REQUIRE(sim.getStepsBelowTolerance() == sim.getStepsToConvergence());
    REQUIRE(sim.getSolverErrormessage() == "");
  }

  SECTION("Run_until_convergence_pixel_absolute_mode") {
    m.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
    simulate::SteadyStateSimulation sim(
        m, abs_stop_tolerance, 10,
        simulate::SteadyStateConvergenceMode::absolute, 100000000, 5.0);

    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::Pixel);
    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getConvergenceMode() ==
            simulate::SteadyStateConvergenceMode::absolute);
    sim.run(); // run until convergence
    REQUIRE(sim.hasConverged());
    REQUIRE(sim.getSolverStopRequested());
    REQUIRE(sim.getLatestStep() > 0.0);
    REQUIRE(sim.getLatestError() < abs_stop_tolerance);
    REQUIRE(sim.getStepsBelowTolerance() == sim.getStepsToConvergence());
    REQUIRE(sim.getSolverErrormessage() == "");
  }

  SECTION("Run_until_convergence_dune") {
    // use a high stop tolerance to make it run faster too
    m.getSimulationSettings().simulatorType = simulate::SimulatorType::DUNE;
    simulate::SteadyStateSimulation sim(
        m, abs_stop_tolerance, 10,
        simulate::SteadyStateConvergenceMode::absolute, 100000000, 5.0);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::DUNE);
    REQUIRE(sim.hasConverged() == false);
    sim.run(); // run until convergence

    REQUIRE(sim.hasConverged());
    // stop_requested is ignored by DUNE, hence not checked
    REQUIRE(sim.getLatestStep() > 0.0);
    REQUIRE(sim.getLatestError() < abs_stop_tolerance);
    REQUIRE(sim.getStepsBelowTolerance() == sim.getStepsToConvergence());
    REQUIRE(sim.getSolverErrormessage() == "");
  }
  SECTION("Reset_restores_state") {
    m.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
    simulate::SteadyStateSimulation sim(
        m, 1e-4, 10, simulate::SteadyStateConvergenceMode::relative, 100000000,
        1.0);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::Pixel);
    REQUIRE(sim.hasConverged() == false);
    sim.run(); // run until convergence
    REQUIRE(sim.hasConverged());
    sim.reset();
    REQUIRE(sim.getStepsBelowTolerance() == 0);
    REQUIRE(sim.getStepsToConvergence() == 10);
    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getStopTolerance() == 1e-4);
    REQUIRE(sim.getDt() == 1.0);
    REQUIRE(sim.getLatestError() == std::numeric_limits<double>::max());
    REQUIRE(sim.getLatestStep() == 0.0);
  }
}
