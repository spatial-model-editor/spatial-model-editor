#include "catch_wrapper.hpp"
#include "model.hpp"
#include "qt_test_utils.hpp"
#include "serialization.hpp"
#include <fstream>
#include <vector>

using namespace sme;

static void createOldSmeFile(const QString &filename) {
  QFile fIn(QString(":/test/smefiles/%1").arg(filename));
  fIn.open(QIODevice::ReadOnly);
  auto data{fIn.readAll()};
  QFile fOut(filename);
  fOut.open(QIODevice::WriteOnly);
  fOut.write(data);
}

SCENARIO("Serialization",
         "[core/common/serialization][core/common][core][serialization]") {
  GIVEN("Import nonexistent file") {
    REQUIRE(utils::importSmeFile("idontexist.txt").xmlModel.empty());
  }
  GIVEN("Import invalid file") {
    std::ofstream fs("invalid.sme", std::ios::binary);
    double x{0.765};
    fs << x;
    fs.close();
    REQUIRE(utils::importSmeFile("invalid.sme").xmlModel.empty());
  }
  GIVEN("Valid v0 smefile (no SimulationSettings)") {
    createOldSmeFile("very-simple-model-v0.sme");
    auto contents{utils::importSmeFile("very-simple-model-v0.sme")};
    REQUIRE(contents.xmlModel.size() == 134371);
    REQUIRE(contents.simulationData.xmlModel.size() == 541828);
    REQUIRE(contents.simulationData.timePoints.size() == 3);
    REQUIRE(contents.simulationData.timePoints[0] == dbl_approx(0.0));
    REQUIRE(contents.simulationData.timePoints[1] == dbl_approx(0.5));
    REQUIRE(contents.simulationData.timePoints[2] == dbl_approx(1.0));
    REQUIRE(contents.simulationData.concentration.size() == 3);
    REQUIRE(contents.simulationData.concentration[2][0].size() == 5441);
    REQUIRE(contents.simulationData.concentration[0][0][1642] == dbl_approx(0));
    REQUIRE(contents.simulationData.concentration[1][0][1642] ==
            dbl_approx(1.5083400377522022672849289e-70));
    REQUIRE(contents.simulationData.concentration[2][0][1642] ==
            dbl_approx(2.3650364527146514603828109e-55));
  }
  GIVEN("Valid v1 smefile (SimulationSettings not stored in model)") {
    sme::simulate::SimulationData data;
    createOldSmeFile("very-simple-model-v1.sme");
    auto contents{utils::importSmeFile("very-simple-model-v1.sme")};
    REQUIRE(contents.simulationData.timePoints.size() == 4);
    REQUIRE(contents.simulationData.timePoints[0] == dbl_approx(0.00));
    REQUIRE(contents.simulationData.timePoints[1] == dbl_approx(0.05));
    REQUIRE(contents.simulationData.timePoints[2] == dbl_approx(0.10));
    REQUIRE(contents.simulationData.timePoints[3] == dbl_approx(0.15));
    REQUIRE(contents.simulationData.concentration.size() == 4);
    REQUIRE(contents.simulationData.concentration[2][0].size() == 5441);
    REQUIRE(contents.simulationData.concentration[0][0][1642] == dbl_approx(0));
    REQUIRE(contents.simulationData.concentration[1][0][1642] ==
            dbl_approx(4.800088138108658530889272e-128));
    REQUIRE(contents.simulationData.concentration[2][0][1642] ==
            dbl_approx(4.7061442226124325927116843e-110));
    REQUIRE(contents.simulationData.concentration[3][0][1642] ==
            dbl_approx(1.06406832003626607985324881e-99));
    // in v1, SimulationSettings are stored in contents
    // but when imported, they are transferred to the sbml doc as xml
    model::Model m;
    m.importSBMLString(contents.xmlModel);
    const auto &options{m.getSimulationSettings().options};
    REQUIRE(m.getSimulationSettings().simulatorType ==
            sme::simulate::SimulatorType::Pixel);
    REQUIRE(m.getSimulationSettings().times.size() == 2);
    REQUIRE(m.getSimulationSettings().times[0].first == 2);
    REQUIRE(m.getSimulationSettings().times[0].second == dbl_approx(0.05));
    REQUIRE(m.getSimulationSettings().times[1].first == 1);
    REQUIRE(m.getSimulationSettings().times[1].second == dbl_approx(0.05));
    REQUIRE(options.dune.integrator == "heun");
    REQUIRE(options.dune.dt == dbl_approx(99.0));
    REQUIRE(options.dune.minDt == dbl_approx(98.0));
    REQUIRE(options.dune.maxDt == dbl_approx(100.0));
    REQUIRE(options.dune.newtonRelErr == dbl_approx(1.0));
    REQUIRE(options.dune.newtonAbsErr == dbl_approx(10.0));
    REQUIRE(options.pixel.integrator ==
            sme::simulate::PixelIntegratorType::RK435);
    REQUIRE(options.pixel.maxErr.rel == dbl_approx(5e-5));
    REQUIRE(options.pixel.maxTimestep == dbl_approx(5.0));
  }
  GIVEN("Valid current sme file") {
    // todo: export & re-import
  }
  GIVEN("settings xml roundtrip") {
    sme::model::Settings annotation{};
    annotation.simulationSettings.times = {{1, 0.3}, {2, 0.1}};
    auto xml{utils::toXml(annotation)};
    auto annotation2{utils::fromXml(xml)};
    REQUIRE(annotation2.simulationSettings.simulatorType == annotation.simulationSettings.simulatorType);
    REQUIRE(annotation2.simulationSettings.times == annotation.simulationSettings.times);
  }
}
