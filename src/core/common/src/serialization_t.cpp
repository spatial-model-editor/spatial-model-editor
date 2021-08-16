#include "catch_wrapper.hpp"
#include "model.hpp"
#include "qt_test_utils.hpp"
#include "serialization.hpp"
#include <fstream>
#include <vector>

using namespace sme;

static void createBinaryFile(const QString &filename) {
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
  GIVEN("Import corrupted file (valid file with some randomly deleted bytes)") {
    createBinaryFile("brusselator-model-corrupted.sme");
    REQUIRE(utils::importSmeFile("brusselator-model-corrupted.sme")
                .xmlModel.empty());
  }
  GIVEN("Import truncated file (valid file with the last few bytes deleted)") {
    createBinaryFile("brusselator-model-truncated.sme");
    REQUIRE(utils::importSmeFile("brusselator-model-truncated.sme")
    .xmlModel.empty());
  }
  GIVEN("Valid v0 smefile (no SimulationSettings)") {
    // v0 smefile was used in spatial-model-editor 1.0.9
    createBinaryFile("very-simple-model-v0.sme");
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
    // v1 smefile was only used in "latest" preview versions
    // of spatial-model-editor between 1.0.9 and 1.1.0 releases
    sme::simulate::SimulationData data;
    createBinaryFile("very-simple-model-v1.sme");
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
  GIVEN("Valid current (v2) sme file") {
    // v2 smefile is used in spatial-model-editor >= 1.1.0
    QFile f(":/models/brusselator-model.xml");
    f.open(QIODevice::ReadOnly);
    model::Model m;
    m.importSBMLString(f.readAll().toStdString());
    m.exportSMEFile("test.sme");
    model::Model m2;
    m2.importFile("test.sme");
    const auto &s{m2.getSimulationSettings()};
    REQUIRE(s.options.pixel.maxErr.rel == dbl_approx(0.005));
  }
  GIVEN("settings xml roundtrip") {
    sme::model::Settings s{};
    s.simulationSettings.times = {{1, 0.3}, {2, 0.1}};
    s.simulationSettings.options.pixel.maxErr.rel = 0.02;
    auto xml{utils::toXml(s)};
    auto s2{utils::fromXml(xml)};
    REQUIRE(s2.simulationSettings.simulatorType ==
            s.simulationSettings.simulatorType);
    REQUIRE(s2.simulationSettings.times == s.simulationSettings.times);
    REQUIRE(s2.simulationSettings.options.pixel.maxErr.rel ==
            dbl_approx(s.simulationSettings.options.pixel.maxErr.rel));
  }
  GIVEN("check DE locale doesn't break settings xml roundtrip") {
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/535
    std::locale userLocale{};
    try {
      userLocale = std::locale::global(std::locale("de_DE.UTF-8"));
    } catch (const std::runtime_error &e) {
      userLocale = std::locale::classic();
    }
    sme::model::Settings s{};
    s.simulationSettings.times = {{1, 0.3}, {2, 0.1}};
    s.simulationSettings.options.pixel.maxErr.rel = 0.02;
    auto xml{utils::toXml(s)};
    auto s2{utils::fromXml(xml)};
    REQUIRE(s2.simulationSettings.times == s.simulationSettings.times);
    REQUIRE(s2.simulationSettings.simulatorType ==
            s.simulationSettings.simulatorType);
    REQUIRE(s2.simulationSettings.options.pixel.maxErr.rel ==
            dbl_approx(s.simulationSettings.options.pixel.maxErr.rel));
    std::locale::global(userLocale);
  }
}
