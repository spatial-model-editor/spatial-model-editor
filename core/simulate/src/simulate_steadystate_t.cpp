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

TEST_CASE("SimulateSteadyState", "[core/simulate/simulate_steadystate]") {

  auto m{getExampleModel(Mod::GrayScott)};
  const std::vector<std::string> comps{"compartment"};
  const std::vector<std::vector<std::string>> specs{{"U", "V"}};
  const double stop_tolerance = 1e-6;

  SECTION("construct") {
    simulate::SteadyStateSimulation sim(
        m, simulate::SimulatorType::Pixel, stop_tolerance, 10,
        simulate::SteadystateConvergenceMode::relative, 1000, 1e-2);
    REQUIRE(sim.getStopTolerance() == stop_tolerance);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::Pixel);
    REQUIRE(sim.getDt() == 1e-2);
    REQUIRE(sim.getStepsBelowTolerance() == 0);
    REQUIRE(sim.getConvergenceMode() ==
            simulate::SteadystateConvergenceMode::relative);
    REQUIRE(sim.getStepsToConvergence() == 10);
    REQUIRE(sim.getTimeout() == 1000);
    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getConcentrations().size() ==
            6298); // number of values for each species in each compartment
                   // concatenated
    REQUIRE(sim.getLatestError().load() == std::numeric_limits<double>::max());
    REQUIRE(sim.getLatestStep().load() == 0.0);
    REQUIRE(sim.getSolverErrormessage() == "");
    REQUIRE(sim.getSolverStopRequested() == false);
    REQUIRE(sim.getCompartmentSpeciesIdxs() ==
            std::vector<std::vector<std::size_t>>{{0, 1}});
    REQUIRE(sim.getCompartmentSpeciesColors()[0][0] != 0);
    REQUIRE(sim.getCompartmentSpeciesColors()[0][1] != 0);
    REQUIRE(sim.getCompartmentSpeciesIds() ==
            std::vector<std::vector<std::string>>{{"U", "V"}});

    auto img = sim.getConcentrationImage(
        std::vector<std::vector<std::size_t>>{{0, 1}}, true);
    REQUIRE(img.empty() == false);
    REQUIRE(img.volume().depth() == 1);
    REQUIRE(img.volume().width() == 100);
    REQUIRE(img.volume().height() == 100);

    sim.setConvergenceMode(simulate::SteadystateConvergenceMode::absolute);
    REQUIRE(sim.getConvergenceMode() ==
            simulate::SteadystateConvergenceMode::absolute);

    sim.setStopTolerance(1e-9);
    REQUIRE(sim.getStopTolerance() == 1e-9);

    sim.setStepsToConvergence(20);
    REQUIRE(sim.getStepsToConvergence() == 20);

    sim.setDt(1e-3);
    REQUIRE(sim.getDt() == 1e-3);

    sim.setTimeout(2000);
    REQUIRE(sim.getTimeout() == 2000);

    sim.setSimulatorType(simulate::SimulatorType::DUNE);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::DUNE);
  }

  SECTION("Run_into_timeout_and_query_for_errormessage") {
    simulate::SteadyStateSimulation sim(
        m, simulate::SimulatorType::Pixel, stop_tolerance, 10,
        simulate::SteadystateConvergenceMode::relative, 50, 1e-2);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::Pixel);
    REQUIRE(sim.hasConverged() == false);
    sim.run(); // run for 500 ms then timeout
    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getStepsBelowTolerance() < sim.getStepsToConvergence());
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
    REQUIRE(sim.getLatestStep().load() > 0.0);
    REQUIRE(sim.getLatestError().load() < stop_tolerance);
    REQUIRE(sim.getStepsBelowTolerance() == sim.getStepsToConvergence());
    REQUIRE(sim.getSolverErrormessage() == "");
  }

  SECTION("Run_until_convergence_pixel_absolute_mode") {
    simulate::SteadyStateSimulation sim(
        m, simulate::SimulatorType::Pixel, 1e-3, 10,
        simulate::SteadystateConvergenceMode::absolute, 100000000, 5.0);

    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::Pixel);
    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getConvergenceMode() ==
            simulate::SteadystateConvergenceMode::absolute);
    sim.run(); // run until convergence
    REQUIRE(sim.hasConverged());
    REQUIRE(sim.getSolverStopRequested());
    REQUIRE(sim.getLatestStep().load() > 0.0);
    REQUIRE(sim.getLatestError().load() < 1e-3);
    REQUIRE(sim.getStepsBelowTolerance() == sim.getStepsToConvergence());
    REQUIRE(sim.getSolverErrormessage() == "");
  }

  SECTION("Run_until_convergence_dune") {
    // we know that it converges, hence timeout is absurd here and we set
    // a large timestep to check for convergence too.
    // use a high stop tolerance to make it run faster too
    simulate::SteadyStateSimulation sim(
        m, simulate::SimulatorType::DUNE, 1e-3, 10,
        simulate::SteadystateConvergenceMode::relative, 100000000, 5.0);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::DUNE);
    REQUIRE(sim.hasConverged() == false);
    sim.run(); // run until convergence

    REQUIRE(sim.hasConverged());
    // stop_requested is ignored by DUNE, hence not checked
    REQUIRE(sim.getLatestStep().load() > 0.0);
    REQUIRE(sim.getLatestError().load() < 1e-3);
    REQUIRE(sim.getStepsBelowTolerance() == sim.getStepsToConvergence());
    REQUIRE(sim.getSolverErrormessage() == "");
  }

  SECTION("change_solver-->simulation_reset") {
    // use a high stop tolerance to make it run faster
    simulate::SteadyStateSimulation sim(
        m, simulate::SimulatorType::Pixel, 1e-3, 10,
        simulate::SteadystateConvergenceMode::relative, 10000000, 5.0);
    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::Pixel);
    sim.run(); // run until convergence
    REQUIRE(sim.hasConverged());
    REQUIRE(sim.getSolverStopRequested());
    REQUIRE(sim.getLatestStep().load() > 0.0);
    REQUIRE(sim.getLatestError().load() < 1e-3);
    REQUIRE(sim.getStepsBelowTolerance() == sim.getStepsToConvergence());

    sim.setSimulatorType(
        simulate::SimulatorType::DUNE); // reset happens when solver is changed

    REQUIRE(sim.getSimulatorType() == simulate::SimulatorType::DUNE);
    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getSolverStopRequested() == false);

    sim.run(); // run until convergence
    REQUIRE(sim.hasConverged() == true);
  }
}
