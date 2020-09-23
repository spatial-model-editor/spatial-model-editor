#include "catch_wrapper.hpp"
#include "cli_simulate.hpp"
#include <QFile>

SCENARIO("CLI Simulate", "[cli][simulate]") {
  cli::Params params;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  auto xml = f.readAll();
  QFile w("tmp.xml");
  w.open(QIODevice::WriteOnly);
  w.write(xml);
  params.filename = "tmp.xml";
  params.simulationTime = 1.0;
  params.imageInterval = 0.5;
  params.simType = simulate::SimulatorType::Pixel;
  doSimulation(params);
  QImage img0("img0.png");
  REQUIRE(img0.size() == QSize(100, 100));
  QImage img1("img1.png");
  REQUIRE(img1.size() == QSize(100, 100));
  QImage img2("img2.png");
  REQUIRE(img2.size() == QSize(100, 100));
}
