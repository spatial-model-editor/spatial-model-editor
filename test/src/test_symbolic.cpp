#include "catch.hpp"
#include "logger.hpp"
#include "symbolic.hpp"

TEST_CASE("5+5: no vars, no constants", "[symbolic][non-gui]") {
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
  std::vector<double> res(1, 0);
  sym.eval(res);
  REQUIRE(res[0] == dbl_approx(10));
}

TEST_CASE("3*x + 7*x: one var, no constants", "[symbolic][non-gui]") {
  std::string expr = "3*x + 7 * x";
  symbolic::Symbolic sym(expr, {"x"}, {});
  CAPTURE(expr);
  REQUIRE(sym.simplify() == "10*x");
  REQUIRE(sym.diff("x") == "10");
  std::vector<double> res(1, 0);
  sym.eval(res, {0.2});
  REQUIRE(res[0] == dbl_approx(2));
  sym.eval(res, {-1e-22});
  REQUIRE(res[0] == dbl_approx(-1e-21));
}

TEST_CASE("3*x + 7*x, 4*x - 3: two expressions, one var, no constants",
          "[symbolic][non-gui]") {
  std::vector<std::string> expr{"3*x + 7 * x", "4*x - 3"};
  symbolic::Symbolic sym(expr, {"x"}, {});
  CAPTURE(expr);
  REQUIRE(sym.simplify() == "10*x");
  REQUIRE(sym.diff("x") == "10");
  std::vector<double> res(2, 0);
  sym.eval(res, {0.2});
  REQUIRE(res[0] == dbl_approx(2));
  REQUIRE(res[1] == dbl_approx(-2.2));
  sym.eval(res, {-1e-22});
  REQUIRE(res[0] == dbl_approx(-1e-21));
  REQUIRE(res[1] == dbl_approx(-4e-22 - 3));
}

TEST_CASE("1.324*x + 2*3: one var, no constants", "[symbolic][non-gui]") {
  std::string expr = "1.324 * x + 2*3";
  symbolic::Symbolic sym(expr, {"x"}, {});
  CAPTURE(expr);
  REQUIRE(sym.simplify() == "6 + 1.324*x");
  REQUIRE(sym.diff("x") == "1.324");
  std::vector<double> res(1, 0);
  sym.eval(res, {0.1});
  REQUIRE(res[0] == dbl_approx(6.1324));
}

TEST_CASE("3*x + 4/x - 1.0*x + 0.2*x*x - 0.1: one var, no constants",
          "[symbolic][non-gui]") {
  std::string expr = "3*x + 4/x - 1.0*x + 0.2*x*x - 0.1";
  symbolic::Symbolic sym(expr, {"x"}, {});
  CAPTURE(expr);
  REQUIRE(sym.simplify() == "-0.1 + 2.0*x + 4*x^(-1) + 0.2*x^2");
  REQUIRE(sym.diff("x") == "2.0 + 0.4*x - 4*x^(-2)");
  std::vector<double> res(1, 0);
  for (auto x : std::vector<double>{-0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
    sym.eval(res, {x});
    REQUIRE(res[0] == dbl_approx(3 * x + 4 / x - 1.0 * x + 0.2 * x * x - 0.1));
  }
}

TEST_CASE("3*x + 4/y - 1.0*x + 0.2*x*y - 0.1: two vars, no constants",
          "[symbolic][non-gui]") {
  std::string expr = "3*x + 4/y - 1.0*x + 0.2*x*y - 0.1";
  symbolic::Symbolic sym(expr, {"x", "y", "z"}, {});
  CAPTURE(expr);
  REQUIRE(sym.simplify() == "-0.1 + 2.0*x + 0.2*x*y + 4*y^(-1)");
  REQUIRE(sym.diff("x") == "2.0 + 0.2*y");
  REQUIRE(sym.diff("y") == "0.2*x - 4*y^(-2)");
  REQUIRE(sym.diff("z") == "0");
  std::vector<double> res(1, 0);
  for (auto x : std::vector<double>{-0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
    for (auto y : std::vector<double>{-0.1, 3, 0.9, 23e-17, 4.88e11, 7e32}) {
      sym.eval(res, {x, y});
      REQUIRE(res[0] ==
              dbl_approx(3 * x + 4 / y - 1.0 * x + 0.2 * x * y - 0.1));
    }
  }
}

TEST_CASE("two expressions, three vars, no constants", "[symbolic][non-gui]") {
  std::vector<std::string> expr = {"3*x + 4/y - 1.0*x + 0.2*x*y - 0.1",
                                   "z - cos(x)*sin(y) - x*y"};
  symbolic::Symbolic sym(expr, {"x", "y", "z"}, {});
  CAPTURE(expr);
  REQUIRE(sym.simplify(0) == "-0.1 + 2.0*x + 0.2*x*y + 4*y^(-1)");
  REQUIRE(sym.simplify(1) == "z - x*y - sin(y)*cos(x)");
  REQUIRE(sym.diff("x", 0) == "2.0 + 0.2*y");
  REQUIRE(sym.diff("y", 0) == "0.2*x - 4*y^(-2)");
  REQUIRE(sym.diff("z", 0) == "0");
  REQUIRE(sym.diff("x", 1) == "-y + sin(y)*sin(x)");
  REQUIRE(sym.diff("y", 1) == "-x - cos(y)*cos(x)");
  REQUIRE(sym.diff("z", 1) == "1");
  std::vector<double> res(2, 0);
  for (auto x : std::vector<double>{-0.1, 0, 0.69, 23e-7, 4.188e5}) {
    for (auto y : std::vector<double>{-0.11, 34, 0.9, 29e-7, 4.88e5}) {
      for (auto z : std::vector<double>{-0.12, 31, 0.89, 37e-7}) {
        sym.eval(res, {x, y, z});
        REQUIRE(res[0] ==
                dbl_approx(3 * x + 4 / y - 1.0 * x + 0.2 * x * y - 0.1));
        REQUIRE(res[1] == dbl_approx(z - cos(x) * sin(y) - x * y));
      }
    }
  }
}

TEST_CASE("e^(4*x): print exponential function", "[symbolic][non-gui]") {
  std::string expr = "e^(4*x)";
  symbolic::Symbolic sym(expr, {"x"}, {});
  CAPTURE(expr);
  REQUIRE(sym.simplify() == "exp(4*x)");
  REQUIRE(sym.diff("x") == "4*exp(4*x)");
  std::vector<double> res(1, 0);
  for (auto x : std::vector<double>{-0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
    sym.eval(res, {x});
    REQUIRE(res[0] == dbl_approx(exp(4 * x)));
  }
}

TEST_CASE("x^(3/2): print square-root function", "[symbolic][non-gui]") {
  std::string expr = "x^(3/2)";
  symbolic::Symbolic sym(expr, {"x"}, {});
  CAPTURE(expr);
  REQUIRE(sym.simplify() == "x^(3/2)");
  REQUIRE(sym.diff("x") == "(3/2)*sqrt(x)");
  std::vector<double> res(1, 0);
  for (auto x : std::vector<double>{0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
    sym.eval(res, {x});
    REQUIRE(res[0] == dbl_approx(x * sqrt(x)));
  }
}

TEST_CASE("3*x + alpha*x - a*n/x: one var, constants", "[symbolic][non-gui]") {
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
  std::vector<double> res(1, 0);
  for (auto x : std::vector<double>{-0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
    sym.eval(res, {x, 2356.546});
    REQUIRE(res[0] == dbl_approx(3 * x + constants["alpha"] * x -
                                 constants["a"] * constants["n"] * x));
  }
}

TEST_CASE("relabel one expression with two vars", "[symbolic][non-gui]") {
  std::string expr = "3*x + 12*sin(y)";
  symbolic::Symbolic sym(expr, {"x", "y"});
  CAPTURE(expr);
  REQUIRE(sym.diff("x") == "3");
  REQUIRE(sym.diff("y") == "12*cos(y)");
  sym.relabel({"newX", "newY"});
  REQUIRE(sym.simplify() == "3*newX + 12*sin(newY)");
  REQUIRE(sym.diff("newX") == "3");
  REQUIRE(sym.diff("newY") == "12*cos(newY)");
}

TEST_CASE("invalid variable relabeling is a no-op", "[symbolic][non-gui]") {
  std::string expr = "3*x + 12*sin(y)";
  symbolic::Symbolic sym(expr, {"x", "y"});
  CAPTURE(expr);
  REQUIRE(sym.simplify() == expr);
  REQUIRE(sym.diff("x") == "3");
  REQUIRE(sym.diff("y") == "12*cos(y)");
  sym.relabel({"a"});
  REQUIRE(sym.simplify() == expr);
  sym.relabel({"a", "b", "c"});
  REQUIRE(sym.simplify() == expr);
}
