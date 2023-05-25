#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include "sme/model.hpp"
#include "sme/serialization.hpp"
#include "sme/utils.hpp"
#include <fstream>
#include <vector>

using namespace sme;

TEST_CASE("Serialization",
          "[core/common/serialization][core/common][core][serialization]") {
  SECTION("Import nonexistent file") {
    REQUIRE(common::importSmeFile("idontexist.txt") == nullptr);
  }
  SECTION("Import invalid file") {
    std::ofstream fs("invalid.sme", std::ios::binary);
    double x{0.765};
    fs << x;
    fs.close();
    REQUIRE(common::importSmeFile("invalid.sme") == nullptr);
  }
  SECTION("Import v2 corrupted file (valid file with some randomly deleted "
          "bytes)") {
    test::createBinaryFile("smefiles/brusselator-model-corrupted.sme",
                           "brusselator-model-corrupted.sme");
    REQUIRE(common::importSmeFile("brusselator-model-corrupted.sme") ==
            nullptr);
  }
  SECTION(
      "Import v2 truncated file (valid file with the last few bytes deleted)") {
    test::createBinaryFile("smefiles/brusselator-model-truncated.sme",
                           "brusselator-model-truncated.sme");
    REQUIRE(common::importSmeFile("brusselator-model-truncated.sme") ==
            nullptr);
  }
  SECTION("Valid v0 smefile (no SimulationSettings)") {
    // v0 smefile was used in spatial-model-editor 1.0.9
    test::createBinaryFile("smefiles/very-simple-model-v0.sme",
                           "very-simple-model-v0.sme");
    auto contents{common::importSmeFile("very-simple-model-v0.sme")};
    REQUIRE(contents->xmlModel.size() == 134371);
    REQUIRE(contents->simulationData->xmlModel.size() == 541828);
    REQUIRE(contents->simulationData->timePoints.size() == 3);
    REQUIRE(contents->simulationData->timePoints[0] == dbl_approx(0.0));
    REQUIRE(contents->simulationData->timePoints[1] == dbl_approx(0.5));
    REQUIRE(contents->simulationData->timePoints[2] == dbl_approx(1.0));
    REQUIRE(contents->simulationData->concentration.size() == 3);
    REQUIRE(contents->simulationData->concentration[2][0].size() == 5441);
    REQUIRE(contents->simulationData->concentration[0][0][1642] ==
            dbl_approx(0));
    REQUIRE(contents->simulationData->concentration[1][0][1642] ==
            dbl_approx(1.5083400377522022672849289e-70));
    REQUIRE(contents->simulationData->concentration[2][0][1642] ==
            dbl_approx(2.3650364527146514603828109e-55));
  }
  SECTION("Valid v1 smefile (SimulationSettings not stored in model)") {
    // v1 smefile was only used in "latest" preview versions
    // of spatial-model-editor between 1.0.9 and 1.1.0 releases
    sme::simulate::SimulationData data;
    test::createBinaryFile("smefiles/very-simple-model-v1.sme",
                           "very-simple-model-v1.sme");
    auto contents{common::importSmeFile("very-simple-model-v1.sme")};
    REQUIRE(contents->simulationData->timePoints.size() == 4);
    REQUIRE(contents->simulationData->timePoints[0] == dbl_approx(0.00));
    REQUIRE(contents->simulationData->timePoints[1] == dbl_approx(0.05));
    REQUIRE(contents->simulationData->timePoints[2] == dbl_approx(0.10));
    REQUIRE(contents->simulationData->timePoints[3] == dbl_approx(0.15));
    REQUIRE(contents->simulationData->concentration.size() == 4);
    REQUIRE(contents->simulationData->concentration[2][0].size() == 5441);
    REQUIRE(contents->simulationData->concentration[0][0][1642] ==
            dbl_approx(0));
    REQUIRE(contents->simulationData->concentration[1][0][1642] ==
            dbl_approx(4.800088138108658530889272e-128));
    REQUIRE(contents->simulationData->concentration[2][0][1642] ==
            dbl_approx(4.7061442226124325927116843e-110));
    REQUIRE(contents->simulationData->concentration[3][0][1642] ==
            dbl_approx(1.06406832003626607985324881e-99));
    // in v1, SimulationSettings are stored in contents
    // but when imported, they are transferred to the sbml doc as xml
    model::Model m;
    m.importSBMLString(contents->xmlModel);
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
  SECTION("Valid v2 sme file") {
    // v2 smefile was used in spatial-model-editor 1.1.0 - 1.1.4 inclusive
    // SimulationData was not wrapped in a unique_ptr
    sme::simulate::SimulationData data;
    test::createBinaryFile("smefiles/very-simple-model-v2.sme",
                           "very-simple-model-v2.sme");
    auto contents{common::importSmeFile("very-simple-model-v2.sme")};
    REQUIRE(contents->simulationData->timePoints.size() == 4);
    REQUIRE(contents->simulationData->timePoints[0] == dbl_approx(0.00));
    REQUIRE(contents->simulationData->timePoints[1] == dbl_approx(0.05));
    REQUIRE(contents->simulationData->timePoints[2] == dbl_approx(0.10));
    REQUIRE(contents->simulationData->timePoints[3] == dbl_approx(0.15));
    REQUIRE(contents->simulationData->concentration.size() == 4);
    REQUIRE(contents->simulationData->concentration[2][0].size() == 5441);
    REQUIRE(contents->simulationData->concentration[0][0][1642] ==
            dbl_approx(0));
    REQUIRE(contents->simulationData->concentration[1][0][1642] ==
            dbl_approx(4.800088138108658530889272e-128));
    REQUIRE(contents->simulationData->concentration[2][0][1642] ==
            dbl_approx(4.7061442226124325927116843e-110));
    REQUIRE(contents->simulationData->concentration[3][0][1642] ==
            dbl_approx(1.06406832003626607985324881e-99));
  }
  SECTION("Valid v3 sme file") {
    // v3 smefile was used in spatial-model-editor 1.1.5 - 1.3.1 inclusive
    // was exported colours instead of indexes in sampledField and
    // sampledFieldColours in Settings
    QFile f(":test/models/brusselator-model_v3.xml");
    f.open(QIODevice::ReadOnly);
    model::Model m;
    m.importSBMLString(f.readAll().toStdString());
    m.exportSMEFile("test.sme");

    model::Model m2;
    m2.importFile("test.sme");
    const auto &s{m2.getSimulationSettings()};
    REQUIRE(s.options.pixel.maxErr.rel == dbl_approx(0.005));
  }
  SECTION("Valid current v4 sme file. The number of colors equals the number "
          "of sampledVolumes") {
    // sme versions > 1.3.1
    QFile f(":/models/very-simple-model.xml");
    f.open(QIODevice::ReadOnly);
    model::Model m;
    m.importSBMLString(f.readAll().toStdString());
    m.exportSMEFile("test2.sme");

    model::Model m2;
    m2.importFile("test2.sme");
    const auto &s{m2.getSampledFieldColours()};
    REQUIRE(s[0] == 4278190592);
    REQUIRE(s[1] == 4287652289);
    REQUIRE(s[2] == 4291134816);
    const auto &s2{m2.getCompartments()};
    REQUIRE(s2.getColours().size() == 3);
    REQUIRE(s2.getColours()[0] == 4278190592);
    REQUIRE(s2.getColours()[1] == 4287652289);
    REQUIRE(s2.getColours()[2] == 4291134816);
  }

  SECTION("Valid current v4 sme file. The number of colors is greater than the "
          "number of sampledVolumes") {
    // sme versions > 1.3.1
    QFile f(":/models/liver-simplified.xml");
    f.open(QIODevice::ReadOnly);
    model::Model m;
    m.importSBMLString(f.readAll().toStdString());
    m.exportSMEFile("test2.sme");

    model::Model m2;
    m2.importFile("test2.sme");
    REQUIRE(m.getGeometry().getImages().volume().depth() == 1);
    REQUIRE(m2.getGeometry().getImages().volume().depth() == 1);
    REQUIRE(m.getGeometry().getImages()[0] == m2.getGeometry().getImages()[0]);
    const auto &s{m2.getSampledFieldColours()};
    REQUIRE(s[0] == 4278719745);
    REQUIRE(s[1] == 4278237440);
    REQUIRE(s[2] == 4279380443);
    const auto &s2{m2.getCompartments()};
    REQUIRE(s2.getColours().size() == 2);
    REQUIRE(s2.getColours()[0] == 4278237440);
    REQUIRE(s2.getColours()[1] == 4279380443);
  }

  SECTION("Import a model saved using the old (1.3.1) format.") {
    // sme versions > 1.3.1
    QFile f(":test/models/liver-simplified_v1.3.1.xml");
    f.open(QIODevice::ReadOnly);
    model::Model m;
    m.importSBMLString(f.readAll().toStdString());
    const auto &s{m.getCompartments()};
    REQUIRE(s.getColours().size() == 2);
    REQUIRE(s.getColours()[0] == 4278237440);
    REQUIRE(s.getColours()[1] == 4279380443);
    m.exportSMEFile("test3.sme");

    model::Model m2;
    m2.importFile("test3.sme");
    REQUIRE(m.getGeometry().getImages().volume().depth() == 1);
    REQUIRE(m2.getGeometry().getImages().volume().depth() == 1);
    REQUIRE(m.getGeometry().getImages()[0] == m2.getGeometry().getImages()[0]);
    const auto &s2{m2.getSampledFieldColours()};
    REQUIRE(s2[0] == 4278719745);
    REQUIRE(s2[1] == 4278237440);
    REQUIRE(s2[2] == 4279380443);
    const auto &s3{m2.getCompartments()};
    REQUIRE(s3.getColours().size() == 2);
    REQUIRE(s3.getColours()[0] == 4278237440);
    REQUIRE(s3.getColours()[1] == 4279380443);
    REQUIRE(s.getCompartments()[0]->nVoxels() ==
            s3.getCompartments()[0]->nVoxels());
    REQUIRE(s.getCompartments()[1]->nVoxels() ==
            s3.getCompartments()[1]->nVoxels());

    m.exportSBMLFile("test4.xml");

    model::Model m3;
    m3.importFile("test4.xml");
    const auto &s4{m3.getSampledFieldColours()};
    REQUIRE(s4[0] == 4278719745);
    REQUIRE(s4[1] == 4278237440);
    REQUIRE(s4[2] == 4279380443);
    const auto &s5{m3.getCompartments()};
    REQUIRE(s5.getColours().size() == 2);
    REQUIRE(s5.getColours()[0] == 4278237440);
    REQUIRE(s5.getColours()[1] == 4279380443);
  }

  SECTION("Import a model saved using the new format with invalid color "
          "annotations") {
    // sme versions > 1.3.1
    QFile f(":test/models/liver-simplified_invalid_colours.xml");
    f.open(QIODevice::ReadOnly);
    model::Model m;
    m.importSBMLString(f.readAll().toStdString());
    const auto &s{m.getSampledFieldColours()};
    REQUIRE(s[0] == qRgb(0, 0, 0));
    REQUIRE(s[1] == common::indexedColours()[0].rgb());
    REQUIRE(s[2] == common::indexedColours()[1].rgb());
    const auto &s2{m.getCompartments()};
    REQUIRE(s2.getColours().size() == 2);
    REQUIRE(s2.getColours()[0] == common::indexedColours()[0].rgb());
    REQUIRE(s2.getColours()[1] == common::indexedColours()[1].rgb());
    m.exportSBMLFile("test5.xml");

    model::Model m3;
    m3.importFile("test5.xml");
    const auto &s3{m3.getSampledFieldColours()};
    REQUIRE(s3[0] == qRgb(0, 0, 0));
    REQUIRE(s3[1] == common::indexedColours()[0].rgb());
    REQUIRE(s3[2] == common::indexedColours()[1].rgb());
    const auto &s4{m3.getCompartments()};
    REQUIRE(s4.getColours().size() == 2);
    REQUIRE(s4.getColours()[0] == common::indexedColours()[0].rgb());
    REQUIRE(s4.getColours()[1] == common::indexedColours()[1].rgb());
  }

  SECTION("Import a model saved using the new format with missing color "
          "annotations") {
    // sme versions > 1.3.1
    QFile f(":test/models/liver-simplified_missing_colours.xml");
    f.open(QIODevice::ReadOnly);
    model::Model m;
    m.importSBMLString(f.readAll().toStdString());
    const auto &s{m.getSampledFieldColours()};
    REQUIRE(s[0] == qRgb(0, 0, 0));
    REQUIRE(s[1] == common::indexedColours()[0].rgb());
    REQUIRE(s[2] == common::indexedColours()[1].rgb());
    const auto &s2{m.getCompartments()};
    REQUIRE(s2.getColours().size() == 2);
    REQUIRE(s2.getColours()[0] == common::indexedColours()[0].rgb());
    REQUIRE(s2.getColours()[1] == common::indexedColours()[1].rgb());
    m.exportSBMLFile("test6.xml");

    model::Model m3;
    m3.importFile("test6.xml");
    const auto &s3{m3.getSampledFieldColours()};
    REQUIRE(s3[0] == qRgb(0, 0, 0));
    REQUIRE(s3[1] == common::indexedColours()[0].rgb());
    REQUIRE(s3[2] == common::indexedColours()[1].rgb());
    const auto &s4{m3.getCompartments()};
    REQUIRE(s4.getColours().size() == 2);
    REQUIRE(s4.getColours()[0] == common::indexedColours()[0].rgb());
    REQUIRE(s4.getColours()[1] == common::indexedColours()[1].rgb());
  }

  SECTION("settings xml roundtrip") {
    sme::model::Settings s{};
    s.simulationSettings.times = {{1, 0.3}, {2, 0.1}};
    s.simulationSettings.options.pixel.maxErr.rel = 0.02;
    auto xml{common::toXml(s)};
    auto s2{common::fromXml(xml)};
    REQUIRE(s2.simulationSettings.simulatorType ==
            s.simulationSettings.simulatorType);
    REQUIRE(s2.simulationSettings.times == s.simulationSettings.times);
    REQUIRE(s2.simulationSettings.options.pixel.maxErr.rel ==
            dbl_approx(s.simulationSettings.options.pixel.maxErr.rel));
  }
  SECTION("check DE locale doesn't break settings xml roundtrip") {
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
    auto xml{common::toXml(s)};
    auto s2{common::fromXml(xml)};
    REQUIRE(s2.simulationSettings.times == s.simulationSettings.times);
    REQUIRE(s2.simulationSettings.simulatorType ==
            s.simulationSettings.simulatorType);
    REQUIRE(s2.simulationSettings.options.pixel.maxErr.rel ==
            dbl_approx(s.simulationSettings.options.pixel.maxErr.rel));
    std::locale::global(userLocale);
  }
}
