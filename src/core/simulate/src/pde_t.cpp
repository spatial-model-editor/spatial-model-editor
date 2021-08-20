#include "catch_wrapper.hpp"
#include "math_test_utils.hpp"
#include "model.hpp"
#include "pde.hpp"
#include "model_test_utils.hpp"

using namespace sme;
using namespace sme::test;

SCENARIO("PDE", "[core/simulate/pde][core/simulate][core][pde]") {
  GIVEN("ABtoC model") {
    auto s{getExampleModel(Mod::ABtoC)};

    std::vector<std::string> speciesIDs{"A", "B", "C"};
    simulate::Reaction reac(&s, speciesIDs, {"r1"});

    REQUIRE(reac.getSpeciesIDs().size() == speciesIDs.size());
    REQUIRE(reac.getSpeciesIDs() == speciesIDs);
    REQUIRE(reac.size() == 1);
    REQUIRE(symEq(reac.getExpression(0), "A * B * k1"));
    REQUIRE_THROWS(reac.getExpression(1));
    REQUIRE(reac.getConstants(0)[5].first == "comp");
    REQUIRE(reac.getConstants(0)[5].second == dbl_approx(3149000.0));
    REQUIRE(reac.getConstants(0)[6].first == "k1");
    REQUIRE(reac.getConstants(0)[6].second == dbl_approx(0.1));
    REQUIRE(reac.getMatrixElement(0, 0) == dbl_approx(-1.0));
    REQUIRE(reac.getMatrixElement(0, 1) == dbl_approx(-1.0));
    REQUIRE(reac.getMatrixElement(0, 2) == dbl_approx(+1.0));
    REQUIRE_THROWS(reac.getMatrixElement(0, 3));
    REQUIRE_THROWS(reac.getMatrixElement(1, 0));
  }
  GIVEN("ABtoC model with invalid reaction rate expression") {
    auto s{getExampleModel(Mod::ABtoC)};
    s.getReactions().add("r2", "comp", "A * A / idontexist");
    s.getReactions().setSpeciesStoichiometry("r2", "A", 1.0);
    s.getReactions().add("r3", "comp", "A / 0");
    s.getReactions().setSpeciesStoichiometry("r3", "A", 1.0);
    std::vector<std::string> speciesIDs{"A", "B", "C"};
    REQUIRE_THROWS_WITH(simulate::Pde(&s, speciesIDs, {"r1", "r2", "r3"}),
                        "Unknown symbol: idontexist");
  }
  GIVEN("simple model") {
    auto s{getTestModel("invalid-dune-names")};
    simulate::Pde pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"});
    REQUIRE(symEq(pde.getRHS()[0], "-1e5*x*dim"));
    REQUIRE(symEq(pde.getRHS()[1], "-1e5*x*dim"));
    REQUIRE(symEq(pde.getRHS()[2], "1e5*x*dim"));
    REQUIRE(symEq(pde.getJacobian()[0][0], "-1e5*x"));
    REQUIRE(symEq(pde.getJacobian()[0][1], "-1e5*dim"));
    REQUIRE(symEq(pde.getJacobian()[0][2], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][3], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][4], "0"));
    REQUIRE(symEq(pde.getJacobian()[2][0], "1e5*x"));
    REQUIRE(symEq(pde.getJacobian()[2][1], "1e5*dim"));
    REQUIRE(symEq(pde.getJacobian()[2][2], "0"));
  }
  GIVEN("simple model with relabeling of variables") {
    auto s{getTestModel("invalid-dune-names")};
    simulate::Pde pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"},
                      {"dim_", "x__", "x_", "cos__", "cos_"});
    REQUIRE(symEq(pde.getRHS()[0], "-1e5*x__*dim_"));
    REQUIRE(symEq(pde.getRHS()[1], "-1e5*x__*dim_"));
    REQUIRE(symEq(pde.getRHS()[2], "1e5*x__*dim_"));
    REQUIRE(symEq(pde.getJacobian()[0][0], "-1e5*x__"));
    REQUIRE(symEq(pde.getJacobian()[0][1], "-1e5*dim_"));
    REQUIRE(symEq(pde.getJacobian()[0][2], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][3], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][4], "0"));
    REQUIRE(symEq(pde.getJacobian()[2][0], "1e5*x__"));
    REQUIRE(symEq(pde.getJacobian()[2][1], "1e5*dim_"));
    REQUIRE(symEq(pde.getJacobian()[2][2], "0"));
  }
  GIVEN("invalid relabeling of variables") {
    THEN("ignore relabeling") {
      auto s{getTestModel("invalid-dune-names")};
      simulate::Pde pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"},
                        {"z", "y"});
      REQUIRE(symEq(pde.getRHS()[0], "-1e5*x*dim"));
      REQUIRE(symEq(pde.getRHS()[1], "-1e5*x*dim"));
      REQUIRE(symEq(pde.getRHS()[2], "1e5*x*dim"));
      REQUIRE(symEq(pde.getJacobian()[0][0], "-1e5*x"));
      REQUIRE(symEq(pde.getJacobian()[0][1], "-1e5*dim"));
      REQUIRE(symEq(pde.getJacobian()[0][2], "0"));
      REQUIRE(symEq(pde.getJacobian()[0][3], "0"));
      REQUIRE(symEq(pde.getJacobian()[0][4], "0"));
      REQUIRE(symEq(pde.getJacobian()[2][0], "1e5*x"));
      REQUIRE(symEq(pde.getJacobian()[2][1], "1e5*dim"));
      REQUIRE(symEq(pde.getJacobian()[2][2], "0"));
    }
  }
  GIVEN("rescaling of reactions & relabelling of variables") {
    auto s{getTestModel("invalid-dune-names")};
    simulate::PdeScaleFactors scaleFactors;
    scaleFactors.reaction = 0.2;
    scaleFactors.species = 1.0;
    simulate::Pde pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"},
                      {"dim_", "x__", "x_", "cos__", "cos_"}, scaleFactors);
    REQUIRE(symEq(pde.getRHS()[0], "-2e4*x__*dim_"));
    REQUIRE(symEq(pde.getRHS()[1], "-2e4*x__*dim_"));
    REQUIRE(symEq(pde.getRHS()[2], "2e4*x__*dim_"));
    REQUIRE(symEq(pde.getJacobian()[0][0], "-2e4*x__"));
    REQUIRE(symEq(pde.getJacobian()[0][1], "-2e4*dim_"));
    REQUIRE(symEq(pde.getJacobian()[0][2], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][3], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][4], "0"));
    REQUIRE(symEq(pde.getJacobian()[2][0], "2e4*x__"));
    REQUIRE(symEq(pde.getJacobian()[2][1], "2e4*dim_"));
    REQUIRE(symEq(pde.getJacobian()[2][2], "0"));
  }
  GIVEN("rescaling of species & reactions") {
    auto s{getTestModel("invalid-dune-names")};
    simulate::PdeScaleFactors scaleFactors;
    scaleFactors.reaction = 0.27;
    scaleFactors.species = 10.0;

    simulate::Pde pde(&s, {"dim", "x", "x_", "cos", "cos_"}, {"r1"}, {},
                      scaleFactors);
    REQUIRE(symEq(pde.getRHS()[0], "-2.7e6*x*dim"));
    REQUIRE(symEq(pde.getRHS()[1], "-2.7e6*x*dim"));
    REQUIRE(symEq(pde.getRHS()[2], "2.7e6*x*dim"));
    REQUIRE(symEq(pde.getJacobian()[0][0], "-2.7e6*x"));
    REQUIRE(symEq(pde.getJacobian()[0][1], "-2.7e6*dim"));
    REQUIRE(symEq(pde.getJacobian()[0][2], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][3], "0"));
    REQUIRE(symEq(pde.getJacobian()[0][4], "0"));
    REQUIRE(symEq(pde.getJacobian()[2][0], "2.7e6*x"));
    REQUIRE(symEq(pde.getJacobian()[2][1], "2.7e6*dim"));
    REQUIRE(symEq(pde.getJacobian()[2][2], "0"));
  }
}
