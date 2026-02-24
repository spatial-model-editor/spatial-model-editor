#include "catch_wrapper.hpp"
#include "cli_params.hpp"
#include "model_test_utils.hpp"
#include "sme/optimize_options.hpp"

using namespace sme;

TEST_CASE("CLI Params", "[cli][params]") {
  REQUIRE(cli::toString(simulate::SimulatorType::DUNE) == "DUNE");
  REQUIRE(cli::toString(simulate::SimulatorType::Pixel) == "Pixel");

  REQUIRE_NOTHROW(cli::printParams(cli::Params{}));

  CLI::App a;
  auto params = cli::setupCLI(a);
  REQUIRE(a.get_description().substr(0, 24) == "Spatial Model Editor CLI");
  REQUIRE(a.get_groups().size() == 1);
  REQUIRE(a.get_options().size() == 4);

  test::createBinaryFile("smefiles/very-simple-model-v3.sme", "x.sme");
  for (std::size_t i = 0; i < simulate::optAlgorithmTypes.size(); ++i) {
    REQUIRE_NOTHROW(a.parse(fmt::format("fit x.sme -a{}", i)));
    REQUIRE(params.fit.algorithm == simulate::optAlgorithmTypes[i]);
  }

  CLI::App b;
  auto simParams = cli::setupCLI(b);
  REQUIRE_NOTHROW(b.parse(
      "simulate x.sme 1 0.1 --simulator pixel --max-threads 3 "
      "--dune-integrator heun --dune-linear-solver superlu "
      "--pixel-integrator rk323 --pixel-enable-multithreading true "
      "--pixel-opt-level 2 --timeout-seconds 10 --throw-on-timeout false "
      "--continue-existing-simulation false"));
  REQUIRE(simParams.simType.has_value());
  REQUIRE(simParams.simType.value() == simulate::SimulatorType::Pixel);
  REQUIRE(simParams.maxThreads.has_value());
  REQUIRE(simParams.maxThreads.value() == 3);
  REQUIRE(simParams.sim.duneIntegrator.has_value());
  REQUIRE(simParams.sim.duneIntegrator.value() == "Heun");
  REQUIRE(simParams.sim.duneLinearSolver.has_value());
  REQUIRE(simParams.sim.duneLinearSolver.value() == "SuperLU");
  REQUIRE(simParams.sim.pixelIntegrator.has_value());
  REQUIRE(simParams.sim.pixelIntegrator.value() ==
          simulate::PixelIntegratorType::RK323);
  REQUIRE(simParams.sim.pixelEnableMultithreading.has_value());
  REQUIRE(simParams.sim.pixelEnableMultithreading.value() == true);
  REQUIRE(simParams.sim.pixelOptLevel.has_value());
  REQUIRE(simParams.sim.pixelOptLevel.value() == 2);
  REQUIRE(simParams.sim.timeoutSeconds == dbl_approx(10));
  REQUIRE(simParams.sim.throwOnTimeout == false);
  REQUIRE(simParams.sim.continueExistingSimulation == false);

  CLI::App c;
  auto simParamsNoTimes = cli::setupCLI(c);
  REQUIRE_NOTHROW(c.parse("simulate x.sme"));
  REQUIRE(simParamsNoTimes.sim.simulationTimes.empty());
  REQUIRE(simParamsNoTimes.sim.imageIntervals.empty());
}
