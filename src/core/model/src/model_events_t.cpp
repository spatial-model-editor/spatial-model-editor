#include "catch_wrapper.hpp"
#include "model.hpp"
#include "model_events.hpp"
#include "model_test_utils.hpp"
#include "sbml_math.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("SBML events",
          "[core/model/events][core/model][core][model][events]") {
  SECTION("SBML: yeast-glycolysis.xml") {
    auto s{getTestModel("yeast-glycolysis")};
    auto &species{s.getSpecies()};
    auto &params{s.getParameters()};
    params.add("param1");
    params.setExpression("param1", "55");
    params.add("param2");
    params.setExpression("param2", "-1.2");
    auto &events{s.getEvents()};
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
    REQUIRE(events.isParameter("n") == true);
    REQUIRE(events.getValue("n") == dbl_approx(0.0));
    // apply event to model
    REQUIRE(params.getExpression("param1") == "55");
    events.applyEvent("n");
    REQUIRE(params.getExpression("param1") == "0");
    // change name
    events.setName("n", "new name");
    REQUIRE(events.getNames().size() == 1);
    REQUIRE(events.getNames()[0] == "new name");
    REQUIRE(events.getName("n") == "new name");
    // change time
    events.setTime("n", 1.4);
    REQUIRE(events.getTime("n") == dbl_approx(1.4));
    // change expression
    events.setExpression("n", "1");
    REQUIRE(events.getExpression("n") == "1");
    REQUIRE(events.isParameter("n") == true);
    REQUIRE(events.getValue("n") == dbl_approx(1.0));
    // apply event to model
    REQUIRE(params.getExpression("param1") == "0");
    events.applyEvent("n");
    REQUIRE(params.getExpression("param1") == "1");
    // change expression to invalid math: no-op
    events.setExpression("n", "invalidmath???");
    REQUIRE(events.getExpression("n") == "1");
    REQUIRE(events.isParameter("n") == true);
    REQUIRE(events.getValue("n") == dbl_approx(1.0));
    // change variable
    events.setVariable("n", "param2");
    REQUIRE(events.getVariable("n") == "param2");
    REQUIRE(events.isParameter("n") == true);
    // apply event to model
    REQUIRE(params.getExpression("param1") == "1");
    REQUIRE(params.getExpression("param2") == "-1.2");
    events.applyEvent("n");
    REQUIRE(params.getExpression("param1") == "1");
    REQUIRE(params.getExpression("param2") == "1");
    // check sbml output
    auto doc{toSbmlDoc(s)};
    const auto *model{doc->getModel()};
    REQUIRE(model->getNumEvents() == 1);
    const auto *event{model->getEvent(0)};
    REQUIRE(event->getId() == "n");
    REQUIRE(event->getName() == "new name");
    REQUIRE(sme::model::mathASTtoString(event->getTrigger()->getMath()) ==
            "time >= 1.4");
    REQUIRE(event->getNumEventAssignments() == 1);
    REQUIRE(event->getEventAssignment(0)->getVariable() == "param2");
    REQUIRE(sme::model::mathASTtoString(
                event->getEventAssignment(0)->getMath()) == "1");
    // change variable to species
    REQUIRE(events.isParameter("n") == true);
    REQUIRE(species.getInitialConcentration("ATP") ==
            dbl_approx(2.52512746499271));
    events.setVariable("n", "ATP");
    REQUIRE(events.isParameter("n") == false);
    events.setExpression("n", "x + y");
    events.applyEvent("n");
    REQUIRE(species.getAnalyticConcentration("ATP") == "x + y");
    // change to non-existent variable: no-op
    events.setHasUnsavedChanges(false);
    events.setVariable("n", "idontexist");
    REQUIRE(events.getHasUnsavedChanges() == false);
    REQUIRE(events.getVariable("n") == "ATP");
    // removing a non-existing event is a no-op
    events.setHasUnsavedChanges(false);
    events.remove("idontexist");
    REQUIRE(events.getHasUnsavedChanges() == false);
    events.remove("n");
    REQUIRE(events.getHasUnsavedChanges() == true);
    REQUIRE(events.getIds().size() == 0);
    REQUIRE(events.getNames().size() == 0);
  }
  SECTION("event with multiple assignments") {
    auto doc{getTestSbmlDoc("yeast-glycolysis")};
    // add event with 3 assignments
    auto *m{doc->getModel()};
    auto *p1{m->createParameter()};
    p1->setConstant(false);
    p1->setId("v1");
    auto *p2{m->createParameter()};
    p2->setConstant(false);
    p2->setId("v2");
    auto *p3{m->createParameter()};
    p3->setConstant(false);
    p3->setId("v3");
    auto *e{m->createEvent()};
    e->setId("e");
    e->setUseValuesFromTriggerTime(true);
    auto *t{e->createTrigger()};
    t->setInitialValue(false);
    t->setPersistent(false);
    // unsupported trigger
    t->setMath(sme::model::mathStringToAST("v1 >= v2").get());
    auto *a1{e->createEventAssignment()};
    a1->setId("a1");
    a1->setVariable("v1");
    a1->setMath(sme::model::mathStringToAST("1").get());
    auto *a2{e->createEventAssignment()};
    a2->setId("a2");
    a2->setVariable("v2");
    a2->setMath(sme::model::mathStringToAST("2").get());
    auto *a3{e->createEventAssignment()};
    a3->setId("a3");
    a3->setVariable("v3");
    a3->setMath(sme::model::mathStringToAST("3").get());
    libsbml::SBMLWriter().writeSBML(doc.get(), "tmpevents.xml");
    model::Model s;
    s.importSBMLFile("tmpevents.xml");
    auto &events = s.getEvents();
    REQUIRE(s.getHasUnsavedChanges() == false);
    REQUIRE(events.getHasUnsavedChanges() == false);
    REQUIRE(events.getIds().size() == 3);
    REQUIRE(events.getNames().size() == 3);
    REQUIRE(events.getVariable("e") == "v1");
    REQUIRE(events.getVariable("e__") == "v2");
    REQUIRE(events.getVariable("e_") == "v3");
    REQUIRE(events.getExpression("e") == "1");
    REQUIRE(events.isParameter("e") == true);
    REQUIRE(events.getValue("e") == dbl_approx(1.0));
    REQUIRE(events.getExpression("e__") == "2");
    REQUIRE(events.isParameter("e__") == true);
    REQUIRE(events.getValue("e__") == dbl_approx(2.0));
    REQUIRE(events.getExpression("e_") == "3");
    REQUIRE(events.isParameter("e_") == true);
    REQUIRE(events.getValue("e_") == dbl_approx(3.0));
    // trigger was unsupported, so replaced with default t >= 0 trigger
    REQUIRE(events.getTime("e") == dbl_approx(0));
    REQUIRE(events.getTime("e__") == dbl_approx(0));
    REQUIRE(events.getTime("e_") == dbl_approx(0));
    // make some changes
    events.setTime("e_", 9.2);
    events.setExpression("e_", "99");
    REQUIRE(events.isParameter("e_") == true);
    REQUIRE(events.getValue("e_") == dbl_approx(99.0));
    s.getParameters().add("param1");
    events.setVariable("e__", "param1");
    REQUIRE(s.getHasUnsavedChanges() == true);
    REQUIRE(events.getHasUnsavedChanges() == true);
    // check sbml output
    auto doc2{toSbmlDoc(s)};
    const auto *model{doc2->getModel()};
    REQUIRE(model->getNumEvents() == 3);
    const auto *e1{model->getEvent("e")};
    const auto *e2{model->getEvent("e__")};
    const auto *e3{model->getEvent("e_")};
    REQUIRE(e1->getNumEventAssignments() == 1);
    REQUIRE(e1->getEventAssignment(0)->getVariable() == "v1");
    REQUIRE(sme::model::mathASTtoString(e1->getEventAssignment(0)->getMath()) ==
            "1");
    REQUIRE(sme::model::mathASTtoString(e2->getTrigger()->getMath()) ==
            "time >= 0");
    REQUIRE(e2->getNumEventAssignments() == 1);
    REQUIRE(e2->getEventAssignment(0)->getVariable() == "param1");
    REQUIRE(sme::model::mathASTtoString(e2->getEventAssignment(0)->getMath()) ==
            "2");
    REQUIRE(sme::model::mathASTtoString(e2->getTrigger()->getMath()) ==
            "time >= 0");
    REQUIRE(e3->getNumEventAssignments() == 1);
    REQUIRE(e3->getEventAssignment(0)->getVariable() == "v3");
    REQUIRE(sme::model::mathASTtoString(e3->getEventAssignment(0)->getMath()) ==
            "99");
    REQUIRE(sme::model::mathASTtoString(e3->getTrigger()->getMath()) ==
            "time >= 9.2");
  }
}
