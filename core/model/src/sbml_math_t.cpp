#include "catch_wrapper.hpp"
#include "math_test_utils.hpp"
#include "model_test_utils.hpp"
#include "sbml_math.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("SBML Math",
          "[core/model/sbml_math][core/model][core][model][sbml_math]") {
  SECTION("Functions can call other functions") {
    auto model{getExampleModel(Mod::ABtoC)};
    auto &functions{model.getFunctions()};
    functions.add("f0");
    functions.setExpression("f0", "66");
    functions.add("f1");
    functions.addArgument("f1", "x");
    functions.setExpression("f1", "2*x");
    REQUIRE(symEq(model::inlineFunctions("f0()", functions), "66"));
    REQUIRE(symEq(model::inlineFunctions("f1(x)", functions), "2*x"));
    REQUIRE(symEq(model::inlineFunctions("f1(2)", functions), "4"));
    REQUIRE(
        symEq(model::inlineFunctions("f1(f1(y))", functions), "2 * (2 * y)"));
    REQUIRE(symEq(model::inlineFunctions("f1(f1(f0()))", functions),
                  "2 * (2 * 66)"));
  }
  SECTION("Function args can have same id as they are called with") {
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/855
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/856
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/857
    auto s{getExampleModel(Mod::ABtoC)};
    auto &funcs{s.getFunctions()};
    funcs.add("f");
    funcs.addArgument("f", "A");
    funcs.addArgument("f", "B");
    funcs.setExpression("f", "A*B");
    REQUIRE(symEq(s.inlineExpr("f(x,y)"), "x*y"));
    REQUIRE(symEq(s.inlineExpr("f(x,2)"), "2*x"));
    REQUIRE(symEq(s.inlineExpr("f(5,y)"), "5*y"));
    REQUIRE(symEq(s.inlineExpr("f(1,B)"), "B"));
    REQUIRE(symEq(s.inlineExpr("f(B,1)"), "B"));
    // in the above libsbml replaces each bvar in turn:
    // A*B -> replace any A with B: B*B -> replace any B with 1 -> 1*1
    REQUIRE(symEq(s.inlineExpr("f(A,B)"), "B*A"));
    REQUIRE(symEq(s.inlineExpr("f(B,A)"), "A*B"));
  }
}
