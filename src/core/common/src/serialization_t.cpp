#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"
#include "serialization.hpp"
#include <fstream>
#include <vector>
using namespace sme;

static void createOldSmeFile(int version, const QString& filename){
  QFile fIn(QString(":/test/smefiles/very-simple-model-v%1.sme").arg(version));
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
  GIVEN("Valid v0 smefile"){
    createOldSmeFile(0, "v0.sme");
    auto contents{utils::importSmeFile("v0.sme")};
    REQUIRE(contents.xmlModel.size() == 134371);
    REQUIRE(contents.simulationData.xmlModel.size() == 541828);
    REQUIRE(contents.simulationData.timePoints.size() == 3);
    REQUIRE(contents.simulationData.timePoints[0] == dbl_approx(0.0));
    REQUIRE(contents.simulationData.timePoints[1] == dbl_approx(0.5));
    REQUIRE(contents.simulationData.timePoints[2] == dbl_approx(1.0));
    REQUIRE(contents.simulationData.concentration.size() == 3);
    REQUIRE(contents.simulationData.concentration[2][0].size() == 5441);
    REQUIRE(contents.simulationData.concentration[0][0][1642] == dbl_approx(0));
    REQUIRE(contents.simulationData.concentration[1][0][1642] == dbl_approx(1.5083400377522022672849289e-70));
    REQUIRE(contents.simulationData.concentration[2][0][1642] == dbl_approx(2.3650364527146514603828109e-55));
  }
  GIVEN("Import/Export fake data") {
    sme::simulate::SimulationData data;
    data.timePoints = {0.0, 1.0};
    data.concentration = {{{1.2, -0.881}, {1.0, -0.1}}};
    data.avgMinMax = {{{{1.0, 2.0, 3.0}, {0.0, 0.1, 0.2}}}};
    data.concentrationMax = {{{1.0, -0.1}, {1.2, -2.1}}};
    data.concPadding = {0, 4};
    data.xmlModel = "sim model";
    sme::simulate::Options options;
    options.dune.increase = 99.9;
    options.pixel.maxErr = {123.4, 0.1234};
    std::vector<std::pair<std::size_t, double>> times{{5, 0.1}, {2, 0.01}};
    utils::SmeFileContents contents{"my model", data, {times, options, sme::simulate::SimulatorType::Pixel}};
    utils::exportSmeFile("test.sme", contents);
    utils::SmeFileContents contents2{};
    REQUIRE(contents2.xmlModel.empty());
    REQUIRE(contents2.simulationData.timePoints.empty());
    REQUIRE(contents2.simulationData.concentration.empty());
    REQUIRE(contents2.simulationData.avgMinMax.empty());
    REQUIRE(contents2.simulationData.concentrationMax.empty());
    REQUIRE(contents2.simulationData.concPadding.empty());
    REQUIRE(contents2.simulationData.xmlModel.empty());
    REQUIRE(contents2.simulationSettings.times.empty());
    // default constructed value:
    REQUIRE(contents2.simulationSettings.simulatorType == sme::simulate::SimulatorType::DUNE);
    contents2 = utils::importSmeFile("test.sme");
    REQUIRE(contents2.xmlModel == "my model");
    REQUIRE(contents2.simulationData.timePoints == data.timePoints);
    REQUIRE(contents2.simulationData.concentration == data.concentration);
    REQUIRE(contents2.simulationData.avgMinMax == data.avgMinMax);
    REQUIRE(contents2.simulationData.concentrationMax == data.concentrationMax);
    REQUIRE(contents2.simulationData.concPadding == data.concPadding);
    REQUIRE(contents2.simulationData.xmlModel == data.xmlModel);
    REQUIRE(contents2.simulationSettings.times == times);
    REQUIRE(contents2.simulationSettings.options.dune.increase == dbl_approx(options.dune.increase));
    REQUIRE(contents2.simulationSettings.options.pixel.maxErr.abs == dbl_approx(options.pixel.maxErr.abs));
    REQUIRE(contents2.simulationSettings.options.pixel.maxErr.rel == dbl_approx(options.pixel.maxErr.rel));
    REQUIRE(contents2.simulationSettings.simulatorType == sme::simulate::SimulatorType::Pixel);
  }
}
