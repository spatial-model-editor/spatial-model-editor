#include "catch_wrapper.hpp"
#include "cli_command.hpp"
#include "sme/model.hpp"
#include <QFile>

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
