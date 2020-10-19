#include "catch_wrapper.hpp"
#include "xml_annotation.hpp"
#include <QFile>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

SCENARIO("XML Annotations",
         "[core/model/xml_annotation][core/"
         "model][core][model][xml_annotation][xml][annotation]") {
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(f.readAll().toStdString().c_str()));
  GIVEN("Display Options annotations") {
    auto *model = doc->getModel();
    // model initially has no display options annotations
    REQUIRE(model::getDisplayOptionsAnnotation(model).has_value() == false);

    // create & save some options
    model::DisplayOptions opts;
    opts.showMinMax = false;
    opts.showSpecies = {true, false, true, true, false};
    opts.normaliseOverAllSpecies = true;
    opts.normaliseOverAllTimepoints = false;
    model::addDisplayOptionsAnnotation(model, opts);

    auto newOpts = model::getDisplayOptionsAnnotation(model);
    REQUIRE(newOpts.has_value());
    REQUIRE(newOpts->showMinMax == opts.showMinMax);
    REQUIRE(newOpts->showSpecies == opts.showSpecies);
    REQUIRE(newOpts->normaliseOverAllSpecies == opts.normaliseOverAllSpecies);
    REQUIRE(newOpts->normaliseOverAllTimepoints ==
            opts.normaliseOverAllTimepoints);

    // save sbml file
    libsbml::writeSBMLToFile(doc.get(), "annotation.xml");

    // save default display options
    auto defOpts = model::DisplayOptions{};
    model::addDisplayOptionsAnnotation(model, defOpts);
    newOpts = model::getDisplayOptionsAnnotation(model);
    REQUIRE(newOpts.has_value());
    REQUIRE(newOpts->showMinMax == defOpts.showMinMax);
    REQUIRE(newOpts->showSpecies == defOpts.showSpecies);
    REQUIRE(newOpts->normaliseOverAllSpecies ==
            defOpts.normaliseOverAllSpecies);
    REQUIRE(newOpts->normaliseOverAllTimepoints ==
            defOpts.normaliseOverAllTimepoints);

    // remove the options annotations
    model::removeDisplayOptionsAnnotation(model);
    REQUIRE(model::getDisplayOptionsAnnotation(model).has_value() == false);

    // remove is no-op if nothing to remove
    model::removeDisplayOptionsAnnotation(model);
    REQUIRE(model::getDisplayOptionsAnnotation(model).has_value() == false);

    // load saved model with annotations
    QFile f2("annotation.xml");
    f2.open(QIODevice::ReadOnly);
    std::unique_ptr<libsbml::SBMLDocument> doc2(
        libsbml::readSBMLFromString(f2.readAll().toStdString().c_str()));
    auto opts2 = model::getDisplayOptionsAnnotation(doc2->getModel());
    REQUIRE(opts2.has_value());
    REQUIRE(opts2->showMinMax == opts.showMinMax);
    REQUIRE(opts2->showSpecies == opts.showSpecies);
    REQUIRE(opts2->normaliseOverAllSpecies == opts.normaliseOverAllSpecies);
    REQUIRE(opts2->normaliseOverAllTimepoints ==
            opts.normaliseOverAllTimepoints);
  }
}
