#include "catch_wrapper.hpp"
#include "math_test_utils.hpp"
#include "model_test_utils.hpp"
#include "sbml_math.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("SBML Math",
          "[core/model/sbml_math][core/model][core][model][sbml_math]") {
  auto model{getExampleModel(Mod::ABtoC)};
  auto &functions{model.getFunctions()};
  functions.add("f0");
  functions.setExpression("f0", "66");
  functions.add("f1");
  functions.addArgument("f1", "x");
  functions.setExpression("f1", "2*x");
  auto doc{toSbmlDoc(model)};
  REQUIRE(model::inlineFunctions("f0()", doc->getModel()) == "(66)");
  REQUIRE(model::inlineFunctions("f1(x)", doc->getModel()) == "(2 * x)");
  REQUIRE(model::inlineFunctions("f1(2)", doc->getModel()) == "(2 * 2)");
  REQUIRE(model::inlineFunctions("f1(f1(y))", doc->getModel()) ==
          "(2 * (2 * y))");
  REQUIRE(model::inlineFunctions("f1(f1(f0()))", doc->getModel()) ==
          "(2 * (2 * 66))");
}
