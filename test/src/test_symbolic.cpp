#include "catch.hpp"
#include "symbolic.hpp"

#include <locale>

TEST_CASE("5+5: no vars, no constants", "[symbolic]") {
  // mathematical expression as string
  std::string expr = "5+5";
  // names of variables in expression
  std::vector<std::string> vars = {};
  // names and values of constants in expression
  std::map<std::string, double> constants = {};
  // parse expression
  symbolic::Symbolic sym(expr, vars, constants);
  // ouput these variables as part of the test
  CAPTURE(expr);
  CAPTURE(vars);
  CAPTURE(constants);
  // simplify expression
  REQUIRE(sym.simplify() == "10");
}

TEST_CASE("3*x + 7*x: one var, no constants", "[symbolic]") {
  std::string expr = "3*x + 7 * x";
  symbolic::Symbolic sym(expr, {"x"}, {});
  CAPTURE(expr);
  REQUIRE(sym.simplify() == "10*x");
  REQUIRE(sym.diff("x") == "10");
}

TEST_CASE("1.324*x + 2*3: one var, no constants", "[symbolic]") {
  std::string expr = "1.324 * x + 2*3";
  symbolic::Symbolic sym(expr, {"x"}, {});
  CAPTURE(expr);
  REQUIRE(sym.simplify() == "6 + 1.324*x");
  REQUIRE(sym.diff("x") == "1.324");
}

TEST_CASE("3*x + 4/x - 1.0*x + 0.2*x*x - 0.1: one var, no constants",
          "[symbolic]") {
  std::string expr = "3*x + 4/x - 1.0*x + 0.2*x*x - 0.1";
  symbolic::Symbolic sym(expr, {"x"}, {});
  CAPTURE(expr);
  REQUIRE(sym.simplify() == "-0.1 + 2.0*x + 4*x^(-1) + 0.2*x^2");
  REQUIRE(sym.diff("x") == "2.0 + 0.4*x - 4*x^(-2)");
}

TEST_CASE("3*x + 4/y - 1.0*x + 0.2*x*y - 0.1: two vars, no constants",
          "[symbolic]") {
  std::string expr = "3*x + 4/y - 1.0*x + 0.2*x*y - 0.1";
  symbolic::Symbolic sym(expr, {"x", "y", "z"}, {});
  CAPTURE(expr);
  REQUIRE(sym.simplify() == "-0.1 + 2.0*x + 0.2*x*y + 4*y^(-1)");
  REQUIRE(sym.diff("x") == "2.0 + 0.2*y");
  REQUIRE(sym.diff("y") == "0.2*x - 4*y^(-2)");
  REQUIRE(sym.diff("z") == "0");
}

TEST_CASE("e^(4*x): print exponential function", "[symbolic]") {
  std::string expr = "e^(4*x)";
  symbolic::Symbolic sym(expr, {"x"}, {});
  CAPTURE(expr);
  REQUIRE(sym.simplify() == "exp(4*x)");
  REQUIRE(sym.diff("x") == "4*exp(4*x)");
}

TEST_CASE("x^(3/2): print square-root function", "[symbolic]") {
  std::string expr = "x^(3/2)";
  symbolic::Symbolic sym(expr, {"x"}, {});
  CAPTURE(expr);
  REQUIRE(sym.simplify() == "x^(3/2)");
  REQUIRE(sym.diff("x") == "(3/2)*sqrt(x)");
}

TEST_CASE("3*x + alpha*x - a*n/x: one var, constants", "[symbolic]") {
  std::map<std::string, double> constants;
  constants["alpha"] = 0.5;
  constants["a"] = 0.8 + 1e-11;
  constants["n"] = -23;
  constants["Unused"] = -99;
  std::string expr = "3*x + alpha*x - a*n*x";
  symbolic::Symbolic sym(expr, {"x", "y"}, constants);
  CAPTURE(expr);
  REQUIRE(sym.simplify() == "21.90000000023*x");
  REQUIRE(sym.diff("x") == "21.90000000023");
  REQUIRE(sym.diff("y") == "0");
}
