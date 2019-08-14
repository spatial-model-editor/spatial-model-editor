#include "reactions.hpp"

#include <QFile>

#include "catch.hpp"

#include "logger.hpp"

TEST_CASE("reactions for ABtoC model", "[reactions][non-gui]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());

  std::vector<std::string> speciesIDs{"A", "B", "C"};
  reactions::Reaction reac(&s, speciesIDs, {"r1"});

  REQUIRE(reac.speciesIDs.size() == speciesIDs.size());
  REQUIRE(reac.speciesIDs == speciesIDs);
  REQUIRE(reac.reacExpressions.size() == 1);
  REQUIRE(reac.reacExpressions[0] == "comp * k1 * A * B");
  REQUIRE(reac.constants[0].at("comp") == dbl_approx(1.0));
  REQUIRE(reac.constants[0].at("k1") == dbl_approx(0.1));
  REQUIRE(reac.M[0][0] == dbl_approx(-1.0));
  REQUIRE(reac.M[0][1] == dbl_approx(-1.0));
  REQUIRE(reac.M[0][2] == dbl_approx(+1.0));
}
