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
    s.getParameters().add("param1");
    s.getParameters().add("param2");
    auto &events = s.getEvents();
    REQUIRE(s.getHasUnsavedChanges() == true);
    REQUIRE(events.getHasUnsavedChanges() == false);
    REQUIRE(events.getIds().size() == 0);
    REQUIRE(events.getNames().size() == 0);
    // add an event
    events.add("n!", "param1");
    REQUIRE(s.getHasUnsavedChanges() == true);
    REQUIRE(events.getHasUnsavedChanges() == true);
    REQUIRE(events.getIds().size() == 1);
    REQUIRE(events.getIds()[0] == "n");
    REQUIRE(events.getNames().size() == 1);
    REQUIRE(events.getNames()[0] == "n!");
    REQUIRE(events.getVariable("n") == "param1");
    REQUIRE(events.getTime("n") == dbl_approx(0));
    REQUIRE(events.getExpression("n") == "0");
    // change name
    events.setName("n", "new name");
    REQUIRE(events.getNames().size() == 1);
    REQUIRE(events.getNames()[0] == "new name");
    REQUIRE(events.getName("n") == "new name");
    // change time
    events.setTime("n", 1.4);
    REQUIRE(events.getTime("n") == dbl_approx(1.4));
    // change expression
    events.setExpression("n", "1    + cos(0)");
    REQUIRE(events.getExpression("n") == "1 + cos(0)");
    // change expression to invalid math: no-op
    events.setExpression("n", "invalidmath???");
    REQUIRE(events.getExpression("n") == "1 + cos(0)");
    // change variable
    events.setVariable("n", "param2");
    REQUIRE(events.getVariable("n") == "param2");
    // change to non-existent variable: no-op
    events.setHasUnsavedChanges(false);
    events.setVariable("n", "idontexist");
    REQUIRE(events.getHasUnsavedChanges() == false);
    REQUIRE(events.getVariable("n") == "param2");
    // removing a non-existing event is a no-op
    events.setHasUnsavedChanges(false);
    events.remove("idontexist");
    REQUIRE(events.getHasUnsavedChanges() == false);
    events.remove("n");
    REQUIRE(events.getHasUnsavedChanges() == true);
    REQUIRE(events.getIds().size() == 0);
    REQUIRE(events.getNames().size() == 0);
//    int k = 0x7fffffff;
//    int kk = k + 1;
//    CAPTURE(k);
//    CAPTURE(kk);
//    REQUIRE(k == kk);
  }
}
