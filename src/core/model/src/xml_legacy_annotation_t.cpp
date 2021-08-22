#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "xml_legacy_annotation.hpp"
#include <QFile>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

using namespace sme;
using namespace sme::test;

SCENARIO("XML Legacy Annotations",
         "[core/model/xml_annotation][core/"
         "model][core][model][xml_annotation][xml][annotation]") {
  QFile f(":test/models/ABtoC-v1.0.9.xml");
  f.open(QIODevice::ReadOnly);
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(f.readAll().toStdString().c_str()));
  auto *model = doc->getModel();
  REQUIRE(model::hasLegacyAnnotations(model) == true);
  auto displayOptions{model::getDisplayOptionsAnnotation(model)};
  REQUIRE(displayOptions.has_value() == true);
  REQUIRE(displayOptions->normaliseOverAllSpecies == false);
  REQUIRE(displayOptions->normaliseOverAllTimepoints == false);
  REQUIRE(displayOptions->showSpecies.size() == 3);
  REQUIRE(displayOptions->showSpecies[0] == false);
  REQUIRE(displayOptions->showSpecies[1] == true);
  REQUIRE(displayOptions->showSpecies[2] == false);
  REQUIRE(displayOptions->showMinMax == true);
  auto *geom{
      dynamic_cast<libsbml::SpatialModelPlugin *>(model->getPlugin("spatial"))
          ->getGeometry()};
  REQUIRE(geom != nullptr);
  auto *pg{dynamic_cast<libsbml::ParametricGeometry *>(
      geom->getGeometryDefinition("parametricGeometry"))};
  REQUIRE(pg != nullptr);
  auto meshParameters{model::getMeshParamsAnnotationData(pg)};
  REQUIRE(meshParameters.has_value() == true);
  REQUIRE(meshParameters->maxAreas.size() == 1);
  REQUIRE(meshParameters->maxAreas[0] == dbl_approx(999));
  REQUIRE(meshParameters->maxPoints.size() == 1);
  REQUIRE(meshParameters->maxPoints[0] == dbl_approx(4));
  auto cA{model::getSpeciesColourAnnotation(model->getSpecies("A"))};
  REQUIRE(cA.has_value());
  REQUIRE(cA.value() == 4290373201);
  auto cB{model::getSpeciesColourAnnotation(model->getSpecies("B"))};
  REQUIRE(cB.has_value());
  REQUIRE(cB.value() == 4287244468);
  auto cC{model::getSpeciesColourAnnotation(model->getSpecies("C"))};
  REQUIRE(cC.has_value());
  REQUIRE(cC.value() == 4294903078);
  REQUIRE(
      model::getSpeciesColourAnnotation(model->getSpecies("D")).has_value() ==
      false);

  // check we correctly import them
  auto settings{model::importAndRemoveLegacyAnnotations(model)};
  REQUIRE(settings.displayOptions.normaliseOverAllSpecies == false);
  REQUIRE(settings.displayOptions.normaliseOverAllTimepoints == false);
  REQUIRE(settings.displayOptions.showSpecies.size() == 3);
  REQUIRE(settings.displayOptions.showSpecies[0] == false);
  REQUIRE(settings.displayOptions.showSpecies[1] == true);
  REQUIRE(settings.displayOptions.showSpecies[2] == false);
  REQUIRE(settings.displayOptions.showMinMax == true);
  REQUIRE(settings.meshParameters.maxAreas.size() == 1);
  REQUIRE(settings.meshParameters.maxAreas[0] == dbl_approx(999));
  REQUIRE(settings.meshParameters.maxPoints.size() == 1);
  REQUIRE(settings.meshParameters.maxPoints[0] == dbl_approx(4));
  REQUIRE(settings.speciesColours.size() == 3);
  REQUIRE(settings.speciesColours["A"] == 4290373201);
  REQUIRE(settings.speciesColours["B"] == 4287244468);
  REQUIRE(settings.speciesColours["C"] == 4294903078);

  // check we have removed the legacy annotations
  REQUIRE(model::hasLegacyAnnotations(model) == false);
  REQUIRE(model::getDisplayOptionsAnnotation(model).has_value() == false);
  REQUIRE(model::getMeshParamsAnnotationData(pg).has_value() == false);
  REQUIRE(
      model::getSpeciesColourAnnotation(model->getSpecies("A")).has_value() ==
      false);
  REQUIRE(
      model::getSpeciesColourAnnotation(model->getSpecies("B")).has_value() ==
      false);
  REQUIRE(
      model::getSpeciesColourAnnotation(model->getSpecies("C")).has_value() ==
      false);
}
