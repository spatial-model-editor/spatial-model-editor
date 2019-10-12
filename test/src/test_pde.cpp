#include <sbml/SBMLTypes.h>

#include <QFile>

#include "catch_wrapper.hpp"
#include "logger.hpp"
#include "pde.hpp"
#include "sbml.hpp"
#include "sbml_test_data/invalid_dune_names.hpp"

TEST_CASE("reactions for ABtoC model", "[pde][non-gui]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());

  std::vector<std::string> speciesIDs{"A", "B", "C"};
  pde::Reaction reac(&s, speciesIDs, {"r1"});

  REQUIRE(reac.getSpeciesIDs().size() == speciesIDs.size());
  REQUIRE(reac.getSpeciesIDs() == speciesIDs);
  REQUIRE(reac.size() == 1);
  REQUIRE(reac.getExpression(0) == "comp * k1 * A * B");
  REQUIRE_THROWS(reac.getExpression(1));
  REQUIRE(reac.getConstants(0).at("comp") == dbl_approx(3149.0));
  REQUIRE(reac.getConstants(0).at("k1") == dbl_approx(0.1));
  REQUIRE_THROWS(reac.getConstants(0).at("not_a_constant"));
  REQUIRE_THROWS(reac.getConstants(1));
  REQUIRE(reac.getMatrixElement(0, 0) == dbl_approx(-3.1756113051762465e-04));
  REQUIRE(reac.getMatrixElement(0, 1) == dbl_approx(-3.1756113051762465e-04));
  REQUIRE(reac.getMatrixElement(0, 2) == dbl_approx(+3.1756113051762465e-04));
  REQUIRE_THROWS(reac.getMatrixElement(0, 3));
  REQUIRE_THROWS(reac.getMatrixElement(1, 0));
}

TEST_CASE("PDE", "[pde][non-gui]") {
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::invalid_dune_names().xml));
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
  sbml::SbmlDocWrapper s;
  s.importSBMLFile("tmp.xml");
  pde::PDE pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"});
  REQUIRE(pde.getRHS()[0] == "-0.1*x*dim");
  REQUIRE(pde.getRHS()[1] == "-0.1*x*dim");
  REQUIRE(pde.getRHS()[2] == "0.1*x*dim");
  REQUIRE(pde.getJacobian()[0][0] == "-0.1*x");
  REQUIRE(pde.getJacobian()[0][1] == "-0.1*dim");
  REQUIRE(pde.getJacobian()[0][2] == "0");
  REQUIRE(pde.getJacobian()[0][3] == "0");
  REQUIRE(pde.getJacobian()[0][4] == "0");
  REQUIRE(pde.getJacobian()[2][0] == "0.1*x");
  REQUIRE(pde.getJacobian()[2][1] == "0.1*dim");
  REQUIRE(pde.getJacobian()[2][2] == "0");
}

TEST_CASE("PDE with relabeling of variables", "[pde][non-gui]") {
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::invalid_dune_names().xml));
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
  sbml::SbmlDocWrapper s;
  s.importSBMLFile("tmp.xml");
  pde::PDE pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"},
               {"dim_", "x__", "x_", "cos__", "cos_"});
  REQUIRE(pde.getRHS()[0] == "-0.1*x__*dim_");
  REQUIRE(pde.getRHS()[1] == "-0.1*x__*dim_");
  REQUIRE(pde.getRHS()[2] == "0.1*x__*dim_");
  REQUIRE(pde.getJacobian()[0][0] == "-0.1*x__");
  REQUIRE(pde.getJacobian()[0][1] == "-0.1*dim_");
  REQUIRE(pde.getJacobian()[0][2] == "0");
  REQUIRE(pde.getJacobian()[0][3] == "0");
  REQUIRE(pde.getJacobian()[0][4] == "0");
  REQUIRE(pde.getJacobian()[2][0] == "0.1*x__");
  REQUIRE(pde.getJacobian()[2][1] == "0.1*dim_");
  REQUIRE(pde.getJacobian()[2][2] == "0");
}

TEST_CASE("PDE: invalid relabeling of variables is ignored",
          "[pde][invalid][non-gui]") {
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::invalid_dune_names().xml));
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
  sbml::SbmlDocWrapper s;
  s.importSBMLFile("tmp.xml");
  pde::PDE pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"}, {"z", "y"});
  REQUIRE(pde.getRHS()[0] == "-0.1*x*dim");
  REQUIRE(pde.getRHS()[1] == "-0.1*x*dim");
  REQUIRE(pde.getRHS()[2] == "0.1*x*dim");
  REQUIRE(pde.getJacobian()[0][0] == "-0.1*x");
  REQUIRE(pde.getJacobian()[0][1] == "-0.1*dim");
  REQUIRE(pde.getJacobian()[0][2] == "0");
  REQUIRE(pde.getJacobian()[0][3] == "0");
  REQUIRE(pde.getJacobian()[0][4] == "0");
  REQUIRE(pde.getJacobian()[2][0] == "0.1*x");
  REQUIRE(pde.getJacobian()[2][1] == "0.1*dim");
  REQUIRE(pde.getJacobian()[2][2] == "0");
}

TEST_CASE("PDE with rescaling of reactions & relabelling of species",
          "[pde][non-gui]") {
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::invalid_dune_names().xml));
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
  sbml::SbmlDocWrapper s;
  s.importSBMLFile("tmp.xml");
  pde::PDE pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"},
               {"dim_", "x__", "x_", "cos__", "cos_"}, {"5"});
  REQUIRE(pde.getRHS()[0] == "-0.02*x__*dim_");
  REQUIRE(pde.getRHS()[1] == "-0.02*x__*dim_");
  REQUIRE(pde.getRHS()[2] == "0.02*x__*dim_");
  REQUIRE(pde.getJacobian()[0][0] == "-0.02*x__");
  REQUIRE(pde.getJacobian()[0][1] == "-0.02*dim_");
  REQUIRE(pde.getJacobian()[0][2] == "0");
  REQUIRE(pde.getJacobian()[0][3] == "0");
  REQUIRE(pde.getJacobian()[0][4] == "0");
  REQUIRE(pde.getJacobian()[2][0] == "0.02*x__");
  REQUIRE(pde.getJacobian()[2][1] == "0.02*dim_");
  REQUIRE(pde.getJacobian()[2][2] == "0");
}

TEST_CASE("PDE: invalid rescaling of reactions is ignored",
          "[pde][invalid][non-gui]") {
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::invalid_dune_names().xml));
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
  sbml::SbmlDocWrapper s;
  s.importSBMLFile("tmp.xml");
  pde::PDE pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"}, {}, {"10", "2"});
  REQUIRE(pde.getRHS()[0] == "-0.1*x*dim");
  REQUIRE(pde.getRHS()[1] == "-0.1*x*dim");
  REQUIRE(pde.getRHS()[2] == "0.1*x*dim");
  REQUIRE(pde.getJacobian()[0][0] == "-0.1*x");
  REQUIRE(pde.getJacobian()[0][1] == "-0.1*dim");
  REQUIRE(pde.getJacobian()[0][2] == "0");
  REQUIRE(pde.getJacobian()[0][3] == "0");
  REQUIRE(pde.getJacobian()[0][4] == "0");
  REQUIRE(pde.getJacobian()[2][0] == "0.1*x");
  REQUIRE(pde.getJacobian()[2][1] == "0.1*dim");
  REQUIRE(pde.getJacobian()[2][2] == "0");
}
