#include "catch.hpp"
#include "catch_qt_ostream.hpp"

#include "numerics.h"

TEST_CASE("evaluate expression: no vars, no constants", "[numerics]") {
  // mathematical expression as string
  std::string expr = "5+5";
  // names of variables in expression
  std::vector<std::string> var_names = {};
  // values of variables (passed by reference)
  std::vector<double> vars = {};
  // names of constants in expression
  std::vector<std::string> constant_names = {};
  // values of constants (passed by copy)
  std::vector<double> constants = {};
  // compile expression
  numerics::ExprEval r(expr, var_names, vars, constant_names, constants);
  // ouput these variables as part of the test
  CAPTURE(expr);
  CAPTURE(var_names);
  CAPTURE(vars);
  CAPTURE(constant_names);
  CAPTURE(constants);
  // evaluate expression
  REQUIRE(r() == Approx(10));
}

TEST_CASE("evaluate expression: pow function", "[numerics]") {
  // mathematical expression as string
  std::string expr = "pow(2, 10)";
  // names of variables in expression
  std::vector<std::string> var_names = {};
  // values of variables (passed by reference)
  std::vector<double> vars = {};
  // names of constants in expression
  std::vector<std::string> constant_names = {};
  // values of constants (passed by copy)
  std::vector<double> constants = {};
  // compile expression
  numerics::ExprEval r(expr, var_names, vars, constant_names, constants);
  // ouput these variables as part of the test
  CAPTURE(expr);
  CAPTURE(var_names);
  CAPTURE(vars);
  CAPTURE(constant_names);
  CAPTURE(constants);
  // evaluate expression
  REQUIRE(r() == Approx(pow(2, 10)));
}

TEST_CASE("evaluate expression: cos, sin functions", "[numerics][non-gui]") {
  // mathematical expression as string
  std::string expr = "cos(1.234)*cos(1.234) + sin(1.234)*sin(1.234)";
  // names of variables in expression
  std::vector<std::string> var_names = {};
  // values of variables (passed by reference)
  std::vector<double> vars = {};
  // names of constants in expression
  std::vector<std::string> constant_names = {};
  // values of constants (passed by copy)
  std::vector<double> constants = {};
  // compile expression
  numerics::ExprEval r(expr, var_names, vars, constant_names, constants);
  // ouput these variables as part of the test
  CAPTURE(expr);
  CAPTURE(var_names);
  CAPTURE(vars);
  CAPTURE(constant_names);
  CAPTURE(constants);
  // evaluate expression
  REQUIRE(r() == Approx(1.0));
}

TEST_CASE("evaluate expression: constants, no vars", "[numerics][non-gui]") {
  std::string expr = "a+b+c";
  std::vector<std::string> var_names = {};
  std::vector<double> vars = {};
  std::vector<std::string> constant_names = {"a", "b", "c"};
  std::vector<double> constants{1, 4, -0.2};
  numerics::ExprEval r(expr, var_names, vars, constant_names, constants);
  CAPTURE(expr);
  CAPTURE(var_names);
  CAPTURE(vars);
  CAPTURE(constant_names);
  CAPTURE(constants);
  REQUIRE(r() == Approx(4.8));
}

TEST_CASE("evaluate expression: vars and constants", "[numerics][non-gui]") {
  GIVEN("expression with vars and constants") {
    std::string expr = "x*c0 + y*cd";
    std::vector<std::string> var_names = {"x", "y"};
    std::vector<double> vars{0.0, 1.0};
    std::vector<std::string> constant_names = {"c0", "cd"};
    std::vector<double> constants{0.5, 0.5};
    numerics::ExprEval r(expr, var_names, vars, constant_names, constants);
    CAPTURE(expr);
    CAPTURE(var_names);
    CAPTURE(vars);
    CAPTURE(constant_names);
    CAPTURE(constants);
    REQUIRE(r() == Approx(0.5));
    WHEN("value of vars change after compiling expression") {
      vars[0] = 1.0;
      THEN("evaluated expression uses updated vars") {
        CAPTURE(expr);
        CAPTURE(var_names);
        CAPTURE(vars);
        CAPTURE(constant_names);
        CAPTURE(constants);
        REQUIRE(r() == Approx(1.0));
      }
    }
    WHEN("value of constants change after compiling expression") {
      constants[0] = 99.0;
      constants[1] = 199.0;
      THEN("evaluated expression does not change") {
        CAPTURE(expr);
        CAPTURE(var_names);
        CAPTURE(vars);
        CAPTURE(constant_names);
        CAPTURE(constants);
        REQUIRE(r() == Approx(0.5));
      }
    }
  }
}
