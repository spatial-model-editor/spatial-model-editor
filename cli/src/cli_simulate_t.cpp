#include "catch_wrapper.hpp"
#include "cli_simulate.hpp"
#include "model.hpp"
#include <QFile>

using namespace sme;

SCENARIO("CLI Simulate", "[cli][simulate]") {
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  auto xml = f.readAll();
  QFile w("tmp.xml");
  w.open(QIODevice::WriteOnly);
  w.write(xml);
  WHEN("Single simulation length, dune sim") {
    cli::Params params;
    params.inputFile = "tmp.xml";
    params.simulationTimes = "0.2";
    params.imageIntervals = "0.1";
    params.outputFile = "tmp.sme";
    params.simType = simulate::SimulatorType::DUNE;
    doSimulation(params);
    model::Model m;
    m.importFile("tmp.sme");
    REQUIRE(m.getSimulationData().timePoints.size() == 3);
    REQUIRE(m.getSimulationData().timePoints[0] == dbl_approx(0.0));
    REQUIRE(m.getSimulationData().timePoints[1] == dbl_approx(0.1));
    REQUIRE(m.getSimulationData().timePoints[2] == dbl_approx(0.2));
  }
  WHEN("Multiple simulation lengths, pixel sim") {
    cli::Params params;
    params.inputFile = "tmp.xml";
    params.simulationTimes = "0.1;0.2;0.3";
    params.imageIntervals = "0.05;0.1;0.15";
    params.outputFile = "tmp.sme";
    params.simType = simulate::SimulatorType::Pixel;
    doSimulation(params);
    model::Model m;
    m.importFile("tmp.sme");
    REQUIRE(m.getSimulationData().timePoints.size() == 7);
    REQUIRE(m.getSimulationData().timePoints[0] == dbl_approx(0.00));
    REQUIRE(m.getSimulationData().timePoints[1] == dbl_approx(0.05));
    REQUIRE(m.getSimulationData().timePoints[2] == dbl_approx(0.10));
    REQUIRE(m.getSimulationData().timePoints[3] == dbl_approx(0.20));
    REQUIRE(m.getSimulationData().timePoints[4] == dbl_approx(0.30));
    REQUIRE(m.getSimulationData().timePoints[5] == dbl_approx(0.45));
    REQUIRE(m.getSimulationData().timePoints[6] == dbl_approx(0.60));
    // repeat using tmp.sme as input file:\
    // simulation continued & results appended
    params.inputFile = "tmp.sme";
    doSimulation(params);
    model::Model m2;
    m2.importFile("tmp.sme");
    REQUIRE(m2.getSimulationData().timePoints.size() == 13);
    REQUIRE(m2.getSimulationData().timePoints[12] == dbl_approx(1.20));
  }
}
