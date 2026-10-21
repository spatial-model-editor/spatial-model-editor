#include "catch_wrapper.hpp"
#include "cli_params.hpp"
#include "model_test_utils.hpp"
#include "sme/optimize_options.hpp"
#include <fstream>

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

TEST_CASE("CLI Params config file", "[cli][params]") {
  test::createBinaryFile("smefiles/very-simple-model-v3.sme", "x.sme");
  CLI::App a;
  auto params = cli::setupCLI(a);
  // dumping the config does not require a subcommand or input file
  REQUIRE_THROWS_AS(a.parse("-d"), CLI::Success);
  auto config = a.config_to_str(true, true);
  // options without a value are commented out
  REQUIRE(config.find("#simulate.simulator=\"\"") != std::string::npos);
  REQUIRE(config.find("#fit.file=") != std::string::npos);
  REQUIRE(config.find("#fit.n-iterations=20") != std::string::npos);
  {
    std::ofstream fs("config.ini");
    fs << config;
  }
  // unedited config file can be read for either subcommand
  for (const auto *subcommand : {"simulate", "fit"}) {
    CLI::App b;
    auto p = cli::setupCLI(b);
    REQUIRE_NOTHROW(b.parse(fmt::format("-c config.ini {} x.sme", subcommand)));
    REQUIRE(p.inputFile == "x.sme");
    REQUIRE(p.simType.has_value() == false);
    REQUIRE(p.maxThreads.has_value() == false);
    REQUIRE(p.fit.nIterations == 20);
  }
  // uncommented options are read from the config file
  for (const auto &[from, to] :
       {std::pair{"#simulate.simulator=\"\"", "simulate.simulator=\"pixel\""},
        std::pair{"#simulate.pixel-integrator=\"\"",
                  "simulate.pixel-integrator=\"rk323\""},
        std::pair{"#fit.n-iterations=20", "fit.n-iterations=5"}}) {
    auto pos = config.find(from);
    REQUIRE(pos != std::string::npos);
    config.replace(pos, std::string(from).size(), to);
  }
  {
    std::ofstream fs("config.ini");
    fs << config;
  }
  CLI::App c;
  auto simParams = cli::setupCLI(c);
  REQUIRE_NOTHROW(c.parse("-c config.ini simulate x.sme"));
  REQUIRE(simParams.simType.has_value());
  REQUIRE(simParams.simType.value() == simulate::SimulatorType::Pixel);
  REQUIRE(simParams.sim.pixelIntegrator.has_value());
  REQUIRE(simParams.sim.pixelIntegrator.value() ==
          simulate::PixelIntegratorType::RK323);
  CLI::App d;
  auto fitParams = cli::setupCLI(d);
  REQUIRE_NOTHROW(d.parse("-c config.ini fit x.sme"));
  REQUIRE(fitParams.fit.nIterations == 5);
}
