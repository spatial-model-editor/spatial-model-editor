#include "catch_wrapper.hpp"
#include "math_test_utils.hpp"
#include "model.hpp"
#include "pde.hpp"
#include "sbml_test_data/invalid_dune_names.hpp"
#include <QFile>
#include <sbml/SBMLDocument.h>
#include <sbml/SBMLReader.h>
#include <sbml/SBMLWriter.h>

using namespace sme;

SCENARIO("PDE", "[core/simulate/pde][core/simulate][core][pde]") {
  GIVEN("ABtoC model") {
    model::Model s;
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    s.importSBMLString(f.readAll().toStdString());

    std::vector<std::string> speciesIDs{"A", "B", "C"};
    simulate::Reaction reac(&s, speciesIDs, {"r1"});

    REQUIRE(reac.getSpeciesIDs().size() == speciesIDs.size());
    REQUIRE(reac.getSpeciesIDs() == speciesIDs);
    REQUIRE(reac.size() == 1);
    REQUIRE(symEq(reac.getExpression(0), "A * B * k1"));
    REQUIRE_THROWS(reac.getExpression(1));
    REQUIRE(reac.getConstants(0)[5].first == "comp");
    REQUIRE(reac.getConstants(0)[5].second == dbl_approx(3149.0));
    REQUIRE(reac.getConstants(0)[6].first == "k1");
    REQUIRE(reac.getConstants(0)[6].second == dbl_approx(0.1));
    REQUIRE(reac.getMatrixElement(0, 0) == dbl_approx(-1.0));
    REQUIRE(reac.getMatrixElement(0, 1) == dbl_approx(-1.0));
    REQUIRE(reac.getMatrixElement(0, 2) == dbl_approx(+1.0));
    REQUIRE_THROWS(reac.getMatrixElement(0, 3));
    REQUIRE_THROWS(reac.getMatrixElement(1, 0));
  }
  GIVEN("simple model") {
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromString(sbml_test_data::invalid_dune_names().xml));
    libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
    model::Model s;
    s.importSBMLFile("tmp.xml");
    simulate::Pde pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"});
    REQUIRE(symEq(pde.getRHS()[0], "-0.1*x*dim"));
    REQUIRE(symEq(pde.getRHS()[1], "-0.1*x*dim"));
    REQUIRE(symEq(pde.getRHS()[2], "0.1*x*dim"));
    REQUIRE(symEq(pde.getJacobian()[0][0], "-0.1*x"));
    REQUIRE(symEq(pde.getJacobian()[0][1], "-0.1*dim"));
    REQUIRE(symEq(pde.getJacobian()[0][2], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][3], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][4], "0"));
    REQUIRE(symEq(pde.getJacobian()[2][0], "0.1*x"));
    REQUIRE(symEq(pde.getJacobian()[2][1], "0.1*dim"));
    REQUIRE(symEq(pde.getJacobian()[2][2], "0"));
  }
  GIVEN("simple model with relabeling of variables") {
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromString(sbml_test_data::invalid_dune_names().xml));
    libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
    model::Model s;
    s.importSBMLFile("tmp.xml");
    simulate::Pde pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"},
                      {"dim_", "x__", "x_", "cos__", "cos_"});
    REQUIRE(symEq(pde.getRHS()[0], "-0.1*x__*dim_"));
    REQUIRE(symEq(pde.getRHS()[1], "-0.1*x__*dim_"));
    REQUIRE(symEq(pde.getRHS()[2], "0.1*x__*dim_"));
    REQUIRE(symEq(pde.getJacobian()[0][0], "-0.1*x__"));
    REQUIRE(symEq(pde.getJacobian()[0][1], "-0.1*dim_"));
    REQUIRE(symEq(pde.getJacobian()[0][2], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][3], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][4], "0"));
    REQUIRE(symEq(pde.getJacobian()[2][0], "0.1*x__"));
    REQUIRE(symEq(pde.getJacobian()[2][1], "0.1*dim_"));
    REQUIRE(symEq(pde.getJacobian()[2][2], "0"));
  }
  GIVEN("invalid relabeling of variables") {
    THEN("ignore relabeling") {
      std::unique_ptr<libsbml::SBMLDocument> doc(libsbml::readSBMLFromString(
          sbml_test_data::invalid_dune_names().xml));
      libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
      model::Model s;
      s.importSBMLFile("tmp.xml");
      simulate::Pde pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"},
                        {"z", "y"});
      REQUIRE(symEq(pde.getRHS()[0], "-0.1*x*dim"));
      REQUIRE(symEq(pde.getRHS()[1], "-0.1*x*dim"));
      REQUIRE(symEq(pde.getRHS()[2], "0.1*x*dim"));
      REQUIRE(symEq(pde.getJacobian()[0][0], "-0.1*x"));
      REQUIRE(symEq(pde.getJacobian()[0][1], "-0.1*dim"));
      REQUIRE(symEq(pde.getJacobian()[0][2], "0"));
      REQUIRE(symEq(pde.getJacobian()[0][3], "0"));
      REQUIRE(symEq(pde.getJacobian()[0][4], "0"));
      REQUIRE(symEq(pde.getJacobian()[2][0], "0.1*x"));
      REQUIRE(symEq(pde.getJacobian()[2][1], "0.1*dim"));
      REQUIRE(symEq(pde.getJacobian()[2][2], "0"));
    }
  }
  GIVEN("rescaling of reactions & relabelling of variables") {
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromString(sbml_test_data::invalid_dune_names().xml));
    libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
    model::Model s;
    s.importSBMLFile("tmp.xml");
    simulate::PdeScaleFactors scaleFactors;
    scaleFactors.reaction = 0.2;
    scaleFactors.species = 1.0;
    simulate::Pde pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"},
                      {"dim_", "x__", "x_", "cos__", "cos_"}, scaleFactors);
    REQUIRE(symEq(pde.getRHS()[0], "-0.02*x__*dim_"));
    REQUIRE(symEq(pde.getRHS()[1], "-0.02*x__*dim_"));
    REQUIRE(symEq(pde.getRHS()[2], "0.02*x__*dim_"));
    REQUIRE(symEq(pde.getJacobian()[0][0], "-0.02*x__"));
    REQUIRE(symEq(pde.getJacobian()[0][1], "-0.02*dim_"));
    REQUIRE(symEq(pde.getJacobian()[0][2], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][3], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][4], "0"));
    REQUIRE(symEq(pde.getJacobian()[2][0], "0.02*x__"));
    REQUIRE(symEq(pde.getJacobian()[2][1], "0.02*dim_"));
    REQUIRE(symEq(pde.getJacobian()[2][2], "0"));
  }
  GIVEN("rescaling of species & reactions") {
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromString(sbml_test_data::invalid_dune_names().xml));
    libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
    model::Model s;
    s.importSBMLFile("tmp.xml");
    simulate::PdeScaleFactors scaleFactors;
    scaleFactors.reaction = 0.27;
    scaleFactors.species = 10.0;

    simulate::Pde pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"}, {},
                      scaleFactors);
    REQUIRE(symEq(pde.getRHS()[0], "-2.7*x*dim"));
    REQUIRE(symEq(pde.getRHS()[1], "-2.7*x*dim"));
    REQUIRE(symEq(pde.getRHS()[2], "2.7*x*dim"));
    REQUIRE(symEq(pde.getJacobian()[0][0], "-2.7*x"));
    REQUIRE(symEq(pde.getJacobian()[0][1], "-2.7*dim"));
    REQUIRE(symEq(pde.getJacobian()[0][2], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][3], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][4], "0"));
    REQUIRE(symEq(pde.getJacobian()[2][0], "2.7*x"));
    REQUIRE(symEq(pde.getJacobian()[2][1], "2.7*dim"));
    REQUIRE(symEq(pde.getJacobian()[2][2], "0"));
  }
}
