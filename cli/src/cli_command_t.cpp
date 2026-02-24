#include "catch_wrapper.hpp"
#include "cli_command.hpp"
#include "sme/model.hpp"
#include <QFile>
#include <optional>

using namespace sme;

TEST_CASE("CLI Simulate", "[cli][simulate]") {
  SECTION("Single simulation length, dune sim") {
    const char *tmpInputFile{"tmpcli1.xml"};
    const char *tmpOutputFile{"tmpcli1.sme"};
    QFile::remove(tmpInputFile);
    QFile::remove(tmpOutputFile);
    QFile::copy(":/models/ABtoC.xml", tmpInputFile);
    cli::Params params;
    params.command = "simulate";
    params.inputFile = tmpInputFile;
    params.sim.simulationTimes = "0.2";
    params.sim.imageIntervals = "0.1";
    params.outputFile = tmpOutputFile;
    params.simType = simulate::SimulatorType::DUNE;
    cli::printParams(params);
    cli::runCommand(params);
    model::Model m;
    m.importFile(tmpOutputFile);
    REQUIRE(m.getSimulationData().timePoints.size() == 3);
    REQUIRE(m.getSimulationData().timePoints[0] == dbl_approx(0.0));
    REQUIRE(m.getSimulationData().timePoints[1] == dbl_approx(0.1));
    REQUIRE(m.getSimulationData().timePoints[2] == dbl_approx(0.2));
  }
  SECTION("Multiple simulation lengths, pixel sim") {
    const char *tmpInputFile{"tmpcli2.xml"};
    const char *tmpOutputFile{"tmpcli2.sme"};
    QFile::remove(tmpInputFile);
    QFile::remove(tmpOutputFile);
    QFile::copy(":/models/ABtoC.xml", tmpInputFile);
    cli::Params params;
    params.command = "simulate";
    params.inputFile = tmpInputFile;
    params.sim.simulationTimes = "0.1;0.2;0.3";
    params.sim.imageIntervals = "0.05;0.1;0.15";
    params.outputFile = tmpOutputFile;
    params.simType = simulate::SimulatorType::Pixel;
    runCommand(params);
    model::Model m;
    m.importFile(tmpOutputFile);
    REQUIRE(m.getSimulationData().timePoints.size() == 7);
    REQUIRE(m.getSimulationData().timePoints[0] == dbl_approx(0.00));
    REQUIRE(m.getSimulationData().timePoints[1] == dbl_approx(0.05));
    REQUIRE(m.getSimulationData().timePoints[2] == dbl_approx(0.10));
    REQUIRE(m.getSimulationData().timePoints[3] == dbl_approx(0.20));
    REQUIRE(m.getSimulationData().timePoints[4] == dbl_approx(0.30));
    REQUIRE(m.getSimulationData().timePoints[5] == dbl_approx(0.45));
    REQUIRE(m.getSimulationData().timePoints[6] == dbl_approx(0.60));
    // repeat using previous output file as input file:
    // simulation continued & results appended
    params.inputFile = tmpOutputFile;
    cli::printParams(params);
    cli::runCommand(params);
    model::Model m2;
    m2.importFile(tmpOutputFile);
    REQUIRE(m2.getSimulationData().timePoints.size() == 13);
    REQUIRE(m2.getSimulationData().timePoints[12] == dbl_approx(1.20));

    // run again, but clear existing simulation results before starting
    params.sim.continueExistingSimulation = false;
    cli::runCommand(params);
    model::Model m3;
    m3.importFile(tmpOutputFile);
    REQUIRE(m3.getSimulationData().timePoints.size() == 7);
    REQUIRE(m3.getSimulationSettings().options.pixel.integrator ==
            simulate::PixelIntegratorType::RK212);

    // and override some Pixel options
    params.sim.pixelIntegrator = simulate::PixelIntegratorType::RK323;
    params.sim.pixelEnableMultithreading = true;
    params.sim.pixelMaxThreads = 1;
    params.sim.pixelOptLevel = 2;
    cli::runCommand(params);
    model::Model m4;
    m4.importFile(tmpOutputFile);
    REQUIRE(m4.getSimulationSettings().options.pixel.integrator ==
            simulate::PixelIntegratorType::RK323);
    REQUIRE(m4.getSimulationSettings().options.pixel.enableMultiThreading ==
            true);
    REQUIRE(m4.getSimulationSettings().options.pixel.maxThreads == 1);
    REQUIRE(m4.getSimulationSettings().options.pixel.optLevel == 2);

    // overriding Pixel max threads should enable multithreading if not explicit
    params.sim.pixelEnableMultithreading = std::nullopt;
    params.sim.pixelMaxThreads = 4;
    cli::runCommand(params);
    model::Model m5;
    m5.importFile(tmpOutputFile);
    REQUIRE(m5.getSimulationSettings().options.pixel.enableMultiThreading ==
            true);
    REQUIRE(m5.getSimulationSettings().options.pixel.maxThreads == 4);

    // omit times and intervals: use model simulation settings from input file
    params.sim.simulationTimes.clear();
    params.sim.imageIntervals.clear();
    params.sim.continueExistingSimulation = false;
    cli::runCommand(params);
    model::Model m6;
    m6.importFile(tmpOutputFile);
    REQUIRE(m6.getSimulationData().timePoints.size() == 7);
    REQUIRE(m6.getSimulationData().timePoints[6] == dbl_approx(0.60));
  }
  SECTION("Invalid partial simulation times") {
    const char *tmpInputFile{"tmpcli4.xml"};
    const char *tmpOutputFile{"tmpcli4.sme"};
    QFile::remove(tmpInputFile);
    QFile::remove(tmpOutputFile);
    QFile::copy(":/models/ABtoC.xml", tmpInputFile);
    cli::Params params;
    params.command = "simulate";
    params.inputFile = tmpInputFile;
    params.sim.simulationTimes = "0.2";
    params.outputFile = tmpOutputFile;
    REQUIRE(cli::runCommand(params) == false);
  }
  SECTION("Parameter fitting") {
    const char *tmpInputFile{"tmpcli3.xml"};
    const char *tmpOutputFile{"tmpcli3.sme"};
    QFile::remove(tmpInputFile);
    QFile::remove(tmpOutputFile);
    QFile::copy(":/models/gray-scott.xml", tmpInputFile);
    cli::Params params;
    params.command = "fit";
    params.inputFile = tmpInputFile;
    params.outputFile = tmpOutputFile;
    params.fit.algorithm = simulate::OptAlgorithmType::PSO;
    params.fit.populationPerThread = 2;
    params.fit.nIterations = 1;
    params.fit.nThreads = 1;
    params.maxThreads = 1;
    params.simType = simulate::SimulatorType::Pixel;
    cli::printParams(params);
    cli::runCommand(params);
  }
}
