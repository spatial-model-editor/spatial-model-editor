#include "catch_wrapper.hpp"
#include "model.hpp"
#include "model_math.hpp"
#include "model_test_utils.hpp"
#include <QFile>
#include <cmath>

using namespace sme;
using namespace sme::test;

TEST_CASE("SBML math", "[core/model/math][core/model][core][model][math]") {
  SECTION("No SBML model, valid expressions") {
    model::ModelMath math;
    REQUIRE(math.isValid() == false);
    REQUIRE(math.getErrorMessage() == "Empty expression");
    math.parse("");
    REQUIRE(math.isValid() == false);
    REQUIRE(math.getErrorMessage() == "Empty expression");
    math.parse("2");
    REQUIRE(math.isValid() == true);
    REQUIRE(math.getErrorMessage().empty());
    REQUIRE(math.eval() == dbl_approx(2));
    math.parse("1.1+2.4");
    REQUIRE(math.isValid() == true);
    REQUIRE(math.getErrorMessage().empty());
    REQUIRE(math.eval() == dbl_approx(3.5));
    math.parse("cos(0)");
    REQUIRE(math.isValid() == true);
    REQUIRE(math.getErrorMessage().empty());
    REQUIRE(math.eval() == dbl_approx(1));
    math.parse("(-2)^2");
    REQUIRE(math.isValid() == true);
    REQUIRE(math.getErrorMessage().empty());
    REQUIRE(math.eval() == dbl_approx(4));
    math.parse("(-2)^2");
    REQUIRE(math.isValid() == true);
    REQUIRE(math.getErrorMessage().empty());
    REQUIRE(math.eval() == dbl_approx(4));
    model::ModelMath math2;
    math2 = std::move(math);
    math2.parse("(-2)^2");
    REQUIRE(math2.isValid() == true);
  }
  SECTION("No SBML model, invalid expressions") {
    model::ModelMath math;
    math.parse("(");
    REQUIRE(math.isValid() == false);
    REQUIRE(math.getErrorMessage() ==
            "Error when parsing input '(' at position 1:  syntax error, "
            "unexpected end of string");
    math.parse("x");
    REQUIRE(math.isValid() == false);
    REQUIRE(math.getErrorMessage() == "Unknown variable: x");
    math.parse("sillyfunction(x)");
    REQUIRE(math.isValid() == false);
    REQUIRE(math.getErrorMessage() == "Unknown function: sillyfunction");
    math.parse("cos(sin(yy))");
    REQUIRE(math.isValid() == false);
    REQUIRE(math.getErrorMessage() == "Unknown variable: yy");
    math.parse("cos(sin(yy)");
    REQUIRE(math.isValid() == false);
    REQUIRE(math.getErrorMessage() ==
            "Error when parsing input 'cos(sin(yy)' at position 11:  syntax "
            "error, unexpected end of string, expecting ')' or ','");
  }
  SECTION("SBML model") {
    auto model{getExampleModel(Mod::ABtoC)};
    auto &funcs = model.getFunctions();
    funcs.add("f1");
    funcs.addArgument("f1", "x");
    funcs.setExpression("f1", "2*x");
    model.getFunctions().add("my_func");
    funcs.setExpression("my_func", "42");
    model.getFunctions().add("my Func!");
    auto &math = model.getMath();
    std::map<const std::string, std::pair<double, bool>> map;
    map["x"] = {1.345, false};
    map["y"] = {-0.9, false};
    SECTION("Valid expressions") {
      math.parse("x");
      REQUIRE(math.isValid() == true);
      REQUIRE(math.eval(map) == dbl_approx(1.345));
      math.parse("x^2 + cos(x)");
      REQUIRE(math.isValid() == true);
      REQUIRE(math.eval(map) == dbl_approx(1.345 * 1.345 + std::cos(1.345)));
      math.parse("x+y");
      REQUIRE(math.isValid() == true);
      REQUIRE(math.eval(map) == dbl_approx(0.445));
      math.parse("f1(x+y)");
      REQUIRE(math.isValid() == true);
      REQUIRE(math.eval(map) == dbl_approx(0.89));
    }
    SECTION("Invalid expressions") {
      math.parse("x(");
      REQUIRE(math.isValid() == false);
      REQUIRE(math.getErrorMessage() ==
              "Error when parsing input 'x(' at position 2:  syntax error, "
              "unexpected end of string");
      math.parse("zz");
      REQUIRE(math.isValid() == false);
      REQUIRE(math.getErrorMessage() == "Unknown variable: zz");
      math.parse("zz(y)");
      REQUIRE(math.isValid() == false);
      REQUIRE(math.getErrorMessage() == "Unknown function: zz");
    }
  }
}
