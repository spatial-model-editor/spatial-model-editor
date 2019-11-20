#include "catch_wrapper.hpp"
#include "logger.hpp"
#include "numerics.hpp"

SCENARIO("Numerics", "[numerics][non-gui]") {
  GIVEN("valid expression: no vars, no constants") {
    // mathematical expression as string
    std::string expr = "5+5";
    // names of variables in expression
    std::vector<std::string> var_names = {};
    // values of variables (passed by reference)
    std::vector<double> vars = {};
    // names and values of constants in expression (passed by copy)
    std::map<std::string, double> constants = {};
    // compile expression
    numerics::ExprEval r(expr, var_names, vars, constants);
    // ouput these variables as part of the test
    CAPTURE(expr);
    CAPTURE(var_names);
    CAPTURE(vars);
    CAPTURE(constants);
    // evaluate expression
    REQUIRE(r() == dbl_approx(10));
  }
  GIVEN("valid expression: pow function") {
    std::string expr = "pow(2, 10)";
    std::vector<double> vars = {};
    numerics::ExprEval r(expr, {}, vars, {});
    CAPTURE(expr);
    CAPTURE(vars);
    REQUIRE(r() == dbl_approx(pow(2, 10)));
  }
  GIVEN("valid expression: cos, sin functions") {
    std::string expr = "cos(1.234)*cos(1.234) + sin(1.234)*sin(1.234)";
    std::vector<double> vars = {};
    numerics::ExprEval r(expr, {}, vars, {});
    CAPTURE(expr);
    CAPTURE(vars);
    REQUIRE(r() == dbl_approx(1.0));
  }
  GIVEN("valid expression: constants, no vars") {
    std::string expr = "a+b+c";
    std::vector<double> vars = {};
    std::map<std::string, double> constants;
    constants["a"] = 1.0;
    constants["b"] = 4.0;
    constants["c"] = -0.2;
    numerics::ExprEval r(expr, {}, vars, constants);
    CAPTURE(expr);
    CAPTURE(vars);
    CAPTURE(constants);
    REQUIRE(r() == dbl_approx(4.8));
  }
  GIVEN("valid expression: vars and constants") {
    std::string expr = "x*c0 + y*cd";
    std::vector<std::string> var_names = {"x", "y"};
    std::vector<double> vars{0.0, 1.0};
    std::map<std::string, double> constants;
    constants["c0"] = 0.5;
    constants["cd"] = 0.5;
    numerics::ExprEval r(expr, var_names, vars, constants);
    CAPTURE(expr);
    CAPTURE(var_names);
    CAPTURE(vars);
    CAPTURE(constants);
    REQUIRE(r() == dbl_approx(0.5));
    WHEN("value of vars change after compiling expression") {
      vars[0] = 1.0;
      THEN("evaluated expression uses updated vars") {
        CAPTURE(expr);
        CAPTURE(var_names);
        CAPTURE(vars);
        CAPTURE(constants);
        REQUIRE(r() == dbl_approx(1.0));
      }
    }
    WHEN("value of constants change after compiling expression") {
      constants["c0"] = 99.9;
      constants["cd"] = 199.0;
      THEN("evaluated expression does not change") {
        CAPTURE(expr);
        CAPTURE(var_names);
        CAPTURE(vars);
        CAPTURE(constants);
        REQUIRE(r() == dbl_approx(0.5));
      }
    }
  }
  GIVEN("expression") {
    THEN("extract symbols") {
      REQUIRE(numerics::getSymbols("") == std::vector<std::string>{});
      REQUIRE(numerics::getSymbols("3-2/14.4") == std::vector<std::string>{});
      REQUIRE(numerics::getSymbols("x+3-2*y") ==
              std::vector<std::string>{"x", "y"});
      REQUIRE(numerics::getSymbols("cos(x) - 12/w") ==
              std::vector<std::string>{"w", "x"});
      REQUIRE(numerics::getSymbols("cos(x) - substrate_34/wTau") ==
              std::vector<std::string>{"substrate_34", "wTau", "x"});
      REQUIRE(numerics::getSymbols("unknown_function(x) - substrate_34/wTau") ==
              std::vector<std::string>{"substrate_34", "unknown_function",
                                       "wTau", "x"});
    }
  }
  GIVEN("invalid expression: undefined symbol") {
    std::string expr = "a+b+c+d";
    std::vector<double> vars = {};
    std::map<std::string, double> constants;
    constants["a"] = 1.0;
    constants["b"] = 4.0;
    constants["c"] = -0.2;
    std::string message =
        "ExprEval::ExprEval : compilation error: ERR190 - Undefined symbol: "
        "'d'";
    REQUIRE_THROWS_WITH(numerics::ExprEval(expr, {}, vars, constants), message);
    CAPTURE(expr);
    CAPTURE(vars);
    CAPTURE(constants);
  }
  GIVEN("invalid expression: mismatched brackets") {
    std::string expr = "a+(b+c";
    std::vector<double> vars = {};
    std::map<std::string, double> constants;
    constants["a"] = 1.0;
    constants["b"] = 4.0;
    constants["c"] = -0.2;
    std::string message =
        "ExprEval::ExprEval : compilation error: ERR004 - Mismatched brackets: "
        "')'";
    REQUIRE_THROWS_WITH(numerics::ExprEval(expr, {}, vars, constants), message);
    CAPTURE(expr);
    CAPTURE(vars);
    CAPTURE(constants);
  }
  GIVEN("invalid expression: constant has same name as a var") {
    std::string expr = "a+b+c";
    std::vector<std::string> var_names = {"a"};
    std::vector<double> vars = {0.3};
    std::map<std::string, double> constants;
    constants["a"] = 1.0;
    constants["b"] = 4.0;
    constants["c"] = -0.2;
    constants["unused1"] = -0.99;
    constants["unused2"] = -3.99;
    std::string message =
        "ExprEval::ExprEval : symbol table error: 'a' already exists";
    REQUIRE_THROWS_WITH(numerics::ExprEval(expr, var_names, vars, constants),
                        message);
    CAPTURE(expr);
    CAPTURE(var_names);
    CAPTURE(vars);
    CAPTURE(constants);
  }
  GIVEN("invalid expression: variable with reserved word as name") {
    std::string expr = "a+b+cos";
    std::vector<std::string> var_names = {"cos"};
    std::vector<double> vars = {0.3};
    std::map<std::string, double> constants;
    constants["a"] = 1.0;
    constants["b"] = 4.0;
    constants["c"] = -0.2;
    std::string message =
        "ExprEval::ExprEval : symbol table error: 'cos' already exists";
    REQUIRE_THROWS_WITH(numerics::ExprEval(expr, var_names, vars, constants),
                        message);
    CAPTURE(expr);
    CAPTURE(var_names);
    CAPTURE(vars);
    CAPTURE(constants);
  }
}
