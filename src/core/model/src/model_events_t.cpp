#include "catch_wrapper.hpp"
#include "model.hpp"
#include "model_events.hpp"
#include "sbml_test_data/yeast_glycolysis.hpp"
#include <QFile>
#include <QImage>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

using namespace sme;

SCENARIO("SBML events",
         "[core/model/events][core/model][core][model][events]") {
  GIVEN("SBML: yeast-glycolysis.xml") {
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromString(sbml_test_data::yeast_glycolysis().xml));
    libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
    model::Model s;
    s.importSBMLFile("tmp.xml");
    auto &events = s.getEvents();
    REQUIRE(s.getHasUnsavedChanges() == false);
    REQUIRE(events.getHasUnsavedChanges() == false);
    REQUIRE(events.getIds().size() == 0);
    REQUIRE(events.getNames().size() == 0);
    events.add("n!", "parameterIdToAlter");
    REQUIRE(s.getHasUnsavedChanges() == true);
    REQUIRE(events.getHasUnsavedChanges() == true);
    REQUIRE(events.getIds().size() == 1);
    REQUIRE(events.getIds()[0] == "n");
    REQUIRE(events.getNames().size() == 1);
    REQUIRE(events.getNames()[0] == "n!");
    // remove non-existing event is no-op
    events.setHasUnsavedChanges(false);
    events.remove("idontexist");
    REQUIRE(s.getHasUnsavedChanges() == false);
    REQUIRE(events.getHasUnsavedChanges() == false);
    events.remove("n");
    REQUIRE(s.getHasUnsavedChanges() == true);
    REQUIRE(events.getHasUnsavedChanges() == true);
    REQUIRE(events.getIds().size() == 0);
    REQUIRE(events.getNames().size() == 0);

  }
}
