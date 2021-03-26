#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"
#include "serialization.hpp"
#include <fstream>
#include <vector>

using namespace sme;

SCENARIO("Serialization",
         "[core/common/serialization][core/common][core][serialization]") {
  GIVEN("Import nonexistent file") {
    utils::SmeFile smeFile;
    REQUIRE(smeFile.importFile("idontexist.txt") == false);
  }
  GIVEN("Import invalid file") {
    std::ofstream fs("invalid.sme", std::ios::binary);
    double x{0.765};
    fs << x;
    fs.close();
    utils::SmeFile smeFile;
    REQUIRE(smeFile.importFile("invalid.sme") == false);
  }
  GIVEN("Import/Export fake data") {
    sme::simulate::SimulationData data;
    data.timePoints = {0.0, 1.0};
    data.concentration = {{{1.2, -0.881}, {1.0, -0.1}}};
    data.avgMinMax = {{{{1.0, 2.0, 3.0}, {0.0, 0.1, 0.2}}}};
    data.concentrationMax = {{{1.0, -0.1}, {1.2, -2.1}}};
    data.concPadding = {0, 4};
    data.xmlModel = "sim model";
    utils::SmeFile smeFile("my model", data);
    smeFile.exportFile("test.sme");
    utils::SmeFile smeFile2;
    REQUIRE(smeFile2.xmlModel().empty());
    REQUIRE(smeFile2.simulationData().timePoints.empty());
    REQUIRE(smeFile2.simulationData().concentration.empty());
    REQUIRE(smeFile2.simulationData().avgMinMax.empty());
    REQUIRE(smeFile2.simulationData().concentrationMax.empty());
    REQUIRE(smeFile2.simulationData().concPadding.empty());
    REQUIRE(smeFile2.simulationData().xmlModel.empty());
    smeFile2.importFile("test.sme");
    auto &data2 = smeFile2.simulationData();
    REQUIRE(smeFile2.xmlModel() == smeFile.xmlModel());
    REQUIRE(data2.timePoints == data.timePoints);
    REQUIRE(data2.concentration == data.concentration);
    REQUIRE(data2.avgMinMax == data.avgMinMax);
    REQUIRE(data2.concentrationMax == data.concentrationMax);
    REQUIRE(data2.concPadding == data.concPadding);
    REQUIRE(data2.xmlModel == data.xmlModel);
  }
  GIVEN("Import/Export fake data") {
    sme::simulate::SimulationData data;
    data.timePoints = {0.0, 1.0};
    data.concentration = {{{1.2, -0.881}, {1.0, -0.1}}};
    data.avgMinMax = {{{{1.0, 2.0, 3.0}, {0.0, 0.1, 0.2}}}};
    data.concentrationMax = {{{1.0, -0.1}, {1.2, -2.1}}};
    data.concPadding = {0, 4};
    data.xmlModel = "sim model";
    utils::SmeFile smeFile("my model", data);
    smeFile.exportFile("test.sme");
    utils::SmeFile smeFile2;
    REQUIRE(smeFile2.xmlModel().empty());
    REQUIRE(smeFile2.simulationData().timePoints.empty());
    REQUIRE(smeFile2.simulationData().concentration.empty());
    REQUIRE(smeFile2.simulationData().avgMinMax.empty());
    REQUIRE(smeFile2.simulationData().concentrationMax.empty());
    REQUIRE(smeFile2.simulationData().concPadding.empty());
    REQUIRE(smeFile2.simulationData().xmlModel.empty());
    smeFile2.importFile("test.sme");
    auto &data2 = smeFile2.simulationData();
    REQUIRE(smeFile2.xmlModel() == smeFile.xmlModel());
    REQUIRE(data2.timePoints == data.timePoints);
    REQUIRE(data2.concentration == data.concentration);
    REQUIRE(data2.avgMinMax == data.avgMinMax);
    REQUIRE(data2.concentrationMax == data.concentrationMax);
    REQUIRE(data2.concPadding == data.concPadding);
    REQUIRE(data2.xmlModel == data.xmlModel);
  }
}
