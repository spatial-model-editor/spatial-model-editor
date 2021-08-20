#include "catch_wrapper.hpp"
#include "model.hpp"
#include "model_parameters.hpp"
#include "model_test_utils.hpp"

using namespace sme;
using namespace sme::test;

SCENARIO("SBML parameters",
         "[core/model/parameters][core/model][core][model][parameters]") {
  GIVEN("SBML: yeast-glycolysis.xml") {
    auto s{getTestModel("yeast-glycolysis")};
    auto &params{s.getParameters()};
    REQUIRE(s.getHasUnsavedChanges() == false);
    REQUIRE(params.getHasUnsavedChanges() == false);
    REQUIRE(params.getIds().size() == 0);
    REQUIRE(params.getNames().size() == 0);
    // 1 compartment + 25 species + 3 space + time
    REQUIRE(params.getSymbols().size() == 30);
    // specify non-existent compartment: species not included
    REQUIRE(params.getSymbols({"idontexist"}).size() == 5);
    // specify multiple compartments including the real one:
    REQUIRE(
        params.getSymbols({"idontexist", "meneither", "compartment"}).size() ==
        30);
    // default geometry spatial coordinates:
    const auto &coords = params.getSpatialCoordinates();
    REQUIRE(coords.x.id == "x");
    REQUIRE(coords.x.name == "x");
    REQUIRE(coords.y.id == "y");
    REQUIRE(coords.y.name == "y");
    WHEN("change spatial coords") {
      auto newC = coords;
      newC.x.name = "x name";
      newC.y.name = "yY";
      REQUIRE(s.getHasUnsavedChanges() == false);
      REQUIRE(params.getHasUnsavedChanges() == false);
      params.setSpatialCoordinates(std::move(newC));
      REQUIRE(s.getHasUnsavedChanges() == true);
      REQUIRE(params.getHasUnsavedChanges() == true);
      REQUIRE(coords.x.id == "x");
      REQUIRE(coords.x.name == "x name");
      REQUIRE(coords.y.id == "y");
      REQUIRE(coords.y.name == "yY");
      auto doc{toSbmlDoc(s)};
      const auto *param{doc->getModel()->getParameter("x")};
      REQUIRE(param->getName() == "x name");
      param = doc->getModel()->getParameter("y");
      REQUIRE(param->getName() == "yY");
    }
    WHEN("add double parameter") {
      REQUIRE(s.getHasUnsavedChanges() == false);
      REQUIRE(params.getHasUnsavedChanges() == false);
      params.add("p1");
      REQUIRE(s.getHasUnsavedChanges() == true);
      REQUIRE(params.getHasUnsavedChanges() == true);
      REQUIRE(params.getIds().size() == 1);
      REQUIRE(params.getIds()[0] == "p1");
      REQUIRE(params.getNames().size() == 1);
      REQUIRE(params.getNames()[0] == "p1");
      REQUIRE(params.getName("p1") == "p1");
      params.setName("p1", "new Name");
      REQUIRE(params.getNames().size() == 1);
      REQUIRE(params.getNames()[0] == "new Name");
      REQUIRE(params.getName("p1") == "new Name");
      // default param is constant with value zero
      auto doc{toSbmlDoc(s)};
      const auto *param{doc->getModel()->getParameter("p1")};
      REQUIRE(param->isSetValue() == true);
      REQUIRE(param->isSetConstant() == true);
      REQUIRE(param->getConstant() == true);
      REQUIRE(param->getValue() == dbl_approx(0));
      // set value
      params.setExpression("p1", "1.4");
      doc = toSbmlDoc(s);
      param = doc->getModel()->getParameter("p1");
      REQUIRE(param->isSetValue() == true);
      REQUIRE(param->isSetConstant() == true);
      REQUIRE(param->getConstant() == true);
      REQUIRE(param->getValue() == dbl_approx(1.4));
      const auto *asgn{doc->getModel()->getAssignmentRuleByVariable("p1")};
      REQUIRE(asgn == nullptr);
      // set another value
      params.setExpression("p1", "-2.7e-5");
      doc = toSbmlDoc(s);
      param = doc->getModel()->getParameter("p1");
      REQUIRE(param->isSetValue() == true);
      REQUIRE(param->isSetConstant() == true);
      REQUIRE(param->getConstant() == true);
      REQUIRE(param->getValue() == dbl_approx(-2.7e-5));
      asgn = doc->getModel()->getAssignmentRuleByVariable("p1");
      REQUIRE(asgn == nullptr);
      // remove param
      params.remove("p1");
      doc = toSbmlDoc(s);
      param = doc->getModel()->getParameter("p1");
      REQUIRE(param == nullptr);
      asgn = doc->getModel()->getAssignmentRuleByVariable("p1");
      REQUIRE(asgn == nullptr);
      REQUIRE(params.getIds().size() == 0);
      REQUIRE(params.getNames().size() == 0);
    }
    WHEN("add math expression parameter") {
      REQUIRE(s.getHasUnsavedChanges() == false);
      REQUIRE(params.getHasUnsavedChanges() == false);
      params.add("p1");
      REQUIRE(s.getHasUnsavedChanges() == true);
      REQUIRE(params.getHasUnsavedChanges() == true);
      params.setHasUnsavedChanges(false);
      REQUIRE(s.getHasUnsavedChanges() == false);
      REQUIRE(params.getHasUnsavedChanges() == false);
      params.setExpression("p1", "cos(1.4)");
      REQUIRE(s.getHasUnsavedChanges() == true);
      REQUIRE(params.getHasUnsavedChanges() == true);
      auto doc{toSbmlDoc(s)};
      const auto *param{doc->getModel()->getParameter("p1")};
      REQUIRE(param->isSetValue() == false);
      REQUIRE(param->isSetConstant() == true);
      REQUIRE(param->getConstant() == false);
      const auto *asgn{doc->getModel()->getAssignmentRuleByVariable("p1")};
      REQUIRE(asgn->getVariable() == "p1");
      REQUIRE(params.getExpression("p1") == "cos(1.4)");
      // change expression to a double: assignment removed
      params.setExpression("p1", "1.4");
      doc = toSbmlDoc(s);
      param = doc->getModel()->getParameter("p1");
      REQUIRE(param->isSetValue() == true);
      REQUIRE(param->isSetConstant() == true);
      REQUIRE(param->getConstant() == true);
      REQUIRE(param->getValue() == dbl_approx(1.4));
      asgn = doc->getModel()->getAssignmentRuleByVariable("p1");
      REQUIRE(asgn == nullptr);
      // change back to an expression
      params.setExpression("p1", "exp(2)  ");
      doc = toSbmlDoc(s);
      param = doc->getModel()->getParameter("p1");
      REQUIRE(param->isSetValue() == false);
      REQUIRE(param->isSetConstant() == true);
      REQUIRE(param->getConstant() == false);
      asgn = doc->getModel()->getAssignmentRuleByVariable("p1");
      REQUIRE(asgn->getVariable() == "p1");
      REQUIRE(params.getExpression("p1") == "exp(2)");
      // remove param
      params.remove("p1");
      doc = toSbmlDoc(s);
      param = doc->getModel()->getParameter("p1");
      REQUIRE(param == nullptr);
      asgn = doc->getModel()->getAssignmentRuleByVariable("p1");
      REQUIRE(asgn == nullptr);
      REQUIRE(params.getIds().size() == 0);
      REQUIRE(params.getNames().size() == 0);
    }
  }
}
