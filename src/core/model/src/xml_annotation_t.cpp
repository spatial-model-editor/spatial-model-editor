#include "catch_wrapper.hpp"
#include "simulate_options.hpp"
#include "xml_annotation.hpp"
#include <QFile>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

using namespace sme;

SCENARIO("XML Annotations",
         "[core/model/xml_annotation][core/"
         "model][core][model][xml_annotation][xml][annotation]") {
  GIVEN("Settings annotations") {
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromString(f.readAll().toStdString().c_str()));
    auto *model = doc->getModel();
    auto settings{model::getSbmlAnnotation(model)};

    // set some options & save
    auto &displayOptions{settings.displayOptions};
    displayOptions.showMinMax = false;
    displayOptions.showSpecies = {true, false, true, true, false};
    displayOptions.normaliseOverAllSpecies = true;
    displayOptions.normaliseOverAllTimepoints = false;
    auto &simulationSettings{settings.simulationSettings};
    simulationSettings.times.push_back({1, 0.2});
    simulationSettings.times.push_back({5, 0.25});
    simulationSettings.options.pixel.maxThreads = 4;
    simulationSettings.options.dune.dt = 0.0123;
    model::setSbmlAnnotation(model, settings);

    // load them from sbml
    auto newSettings{model::getSbmlAnnotation(model)};
    auto &newDisplayOpts{newSettings.displayOptions};
    REQUIRE(newDisplayOpts.showMinMax == displayOptions.showMinMax);
    REQUIRE(newDisplayOpts.showSpecies == displayOptions.showSpecies);
    REQUIRE(newDisplayOpts.normaliseOverAllSpecies ==
            displayOptions.normaliseOverAllSpecies);
    REQUIRE(newDisplayOpts.normaliseOverAllTimepoints ==
            displayOptions.normaliseOverAllTimepoints);
    auto &newSimulationSettings{newSettings.simulationSettings};
    REQUIRE(newSimulationSettings.times.size() == 2);
    REQUIRE(newSimulationSettings.options.pixel.maxThreads == 4);
    REQUIRE(newSimulationSettings.options.dune.dt == dbl_approx(0.0123));

    // change options, save & write to file
    newSimulationSettings.times.push_back({7, 0.33});
    newSimulationSettings.options.pixel.maxThreads = 16;
    model::setSbmlAnnotation(model, newSettings);
    libsbml::writeSBMLToFile(doc.get(), "settings.xml");

    // load saved model with annotations
    QFile f2("settings.xml");
    f2.open(QIODevice::ReadOnly);
    std::unique_ptr<libsbml::SBMLDocument> doc2(
        libsbml::readSBMLFromString(f2.readAll().toStdString().c_str()));
    auto settings2{model::getSbmlAnnotation(doc2->getModel())};
    auto &displayOptions2{settings2.displayOptions};
    REQUIRE(displayOptions2.showMinMax == displayOptions.showMinMax);
    REQUIRE(displayOptions2.showSpecies == displayOptions.showSpecies);
    REQUIRE(displayOptions2.normaliseOverAllSpecies ==
            displayOptions.normaliseOverAllSpecies);
    REQUIRE(displayOptions2.normaliseOverAllTimepoints ==
            displayOptions.normaliseOverAllTimepoints);
    auto &simulationSettings2{settings2.simulationSettings};
    REQUIRE(simulationSettings2.times.size() == 3);
    REQUIRE(simulationSettings2.options.pixel.maxThreads == 16);
    REQUIRE(simulationSettings2.options.dune.dt == dbl_approx(0.0123));
  }
  GIVEN("Invalid settings annotations") {
    QFile f(":test/models/ABtoC-invalid-annotation.xml");
    f.open(QIODevice::ReadOnly);
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromString(f.readAll().toStdString().c_str()));
    auto *model = doc->getModel();
    auto settings{model::getSbmlAnnotation(model)};
    // default-constructed Settings is returned if xml annotation is invalid
    REQUIRE(settings.meshParameters.maxPoints.empty());
    REQUIRE(settings.meshParameters.maxAreas.empty());
    REQUIRE(settings.displayOptions.showSpecies.empty());
    REQUIRE(settings.simulationSettings.times.empty());
    REQUIRE(settings.speciesColours.empty());
  }
}
