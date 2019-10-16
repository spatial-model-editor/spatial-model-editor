#include <QFile>

#include "catch_wrapper.hpp"
#include "logger.hpp"
#include "reactions.hpp"
#include "sbml.hpp"

TEST_CASE("reactions for ABtoC model", "[reactions][non-gui]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());

  std::vector<std::string> speciesIDs{"A", "B", "C"};
  reactions::Reaction reac(&s, speciesIDs, {"r1"});

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
