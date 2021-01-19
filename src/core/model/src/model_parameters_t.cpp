#include "catch_wrapper.hpp"
#include "model.hpp"
#include "model_parameters.hpp"
#include "sbml_test_data/yeast_glycolysis.hpp"
#include <QFile>
#include <QImage>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

using namespace sme;

SCENARIO("SBML parameters",
         "[core/model/parameters][core/model][core][model][parameters]") {
  GIVEN("SBML: yeast-glycolysis.xml") {
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromString(sbml_test_data::yeast_glycolysis().xml));
    libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
    model::Model s;
    s.importSBMLFile("tmp.xml");
    auto &params = s.getParameters();
    REQUIRE(params.getIds().size() == 0);
    REQUIRE(params.getNames().size() == 0);
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
      params.setSpatialCoordinates(std::move(newC));
      REQUIRE(coords.x.id == "x");
      REQUIRE(coords.x.name == "x name");
      REQUIRE(coords.y.id == "y");
      REQUIRE(coords.y.name == "yY");
      std::string xml = s.getXml().toStdString();
      std::unique_ptr<libsbml::SBMLDocument> doc2{
          libsbml::readSBMLFromString(xml.c_str())};
      const auto *param = doc2->getModel()->getParameter("x");
      REQUIRE(param->getName() == "x name");
      param = doc2->getModel()->getParameter("y");
      REQUIRE(param->getName() == "yY");
    }
    WHEN("add double parameter") {
      params.add("p1");
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
      std::string xml = s.getXml().toStdString();
      std::unique_ptr<libsbml::SBMLDocument> doc2{
          libsbml::readSBMLFromString(xml.c_str())};
      const auto *param = doc2->getModel()->getParameter("p1");
      REQUIRE(param->isSetValue() == true);
      REQUIRE(param->isSetConstant() == true);
      REQUIRE(param->getConstant() == true);
      REQUIRE(param->getValue() == dbl_approx(0));
      // set value
      params.setExpression("p1", "1.4");
      xml = s.getXml().toStdString();
      doc2.reset(libsbml::readSBMLFromString(xml.c_str()));
      param = doc2->getModel()->getParameter("p1");
      REQUIRE(param->isSetValue() == true);
      REQUIRE(param->isSetConstant() == true);
      REQUIRE(param->getConstant() == true);
      REQUIRE(param->getValue() == dbl_approx(1.4));
      const auto *asgn = doc2->getModel()->getAssignmentRuleByVariable("p1");
      REQUIRE(asgn == nullptr);
      // set another value
      params.setExpression("p1", "-2.7e-5");
      xml = s.getXml().toStdString();
      doc2.reset(libsbml::readSBMLFromString(xml.c_str()));
      param = doc2->getModel()->getParameter("p1");
      REQUIRE(param->isSetValue() == true);
      REQUIRE(param->isSetConstant() == true);
      REQUIRE(param->getConstant() == true);
      REQUIRE(param->getValue() == dbl_approx(-2.7e-5));
      asgn = doc2->getModel()->getAssignmentRuleByVariable("p1");
      REQUIRE(asgn == nullptr);
      // remove param
      params.remove("p1");
      xml = s.getXml().toStdString();
      doc2.reset(libsbml::readSBMLFromString(xml.c_str()));
      param = doc2->getModel()->getParameter("p1");
      REQUIRE(param == nullptr);
      asgn = doc2->getModel()->getAssignmentRuleByVariable("p1");
      REQUIRE(asgn == nullptr);
      REQUIRE(params.getIds().size() == 0);
      REQUIRE(params.getNames().size() == 0);
    }
    WHEN("add math expression parameter") {
      params.add("p1");
      params.setExpression("p1", "cos(1.4)");
      std::string xml = s.getXml().toStdString();
      std::unique_ptr<libsbml::SBMLDocument> doc2{
          libsbml::readSBMLFromString(xml.c_str())};
      const auto *param = doc2->getModel()->getParameter("p1");
      REQUIRE(param->isSetValue() == false);
      REQUIRE(param->isSetConstant() == true);
      REQUIRE(param->getConstant() == false);
      const auto *asgn = doc2->getModel()->getAssignmentRuleByVariable("p1");
      REQUIRE(asgn->getVariable() == "p1");
      REQUIRE(params.getExpression("p1") == "cos(1.4)");
      // change expression to a double: assignment removed
      params.setExpression("p1", "1.4");
      xml = s.getXml().toStdString();
      doc2.reset(libsbml::readSBMLFromString(xml.c_str()));
      param = doc2->getModel()->getParameter("p1");
      REQUIRE(param->isSetValue() == true);
      REQUIRE(param->isSetConstant() == true);
      REQUIRE(param->getConstant() == true);
      REQUIRE(param->getValue() == dbl_approx(1.4));
      asgn = doc2->getModel()->getAssignmentRuleByVariable("p1");
      REQUIRE(asgn == nullptr);
      // change back to an expression
      params.setExpression("p1", "exp(2)  ");
      xml = s.getXml().toStdString();
      doc2.reset(libsbml::readSBMLFromString(xml.c_str()));
      param = doc2->getModel()->getParameter("p1");
      REQUIRE(param->isSetValue() == false);
      REQUIRE(param->isSetConstant() == true);
      REQUIRE(param->getConstant() == false);
      asgn = doc2->getModel()->getAssignmentRuleByVariable("p1");
      REQUIRE(asgn->getVariable() == "p1");
      REQUIRE(params.getExpression("p1") == "exp(2)");
      // remove param
      params.remove("p1");
      xml = s.getXml().toStdString();
      doc2.reset(libsbml::readSBMLFromString(xml.c_str()));
      param = doc2->getModel()->getParameter("p1");
      REQUIRE(param == nullptr);
      asgn = doc2->getModel()->getAssignmentRuleByVariable("p1");
      REQUIRE(asgn == nullptr);
      REQUIRE(params.getIds().size() == 0);
      REQUIRE(params.getNames().size() == 0);
    }
  }
}
