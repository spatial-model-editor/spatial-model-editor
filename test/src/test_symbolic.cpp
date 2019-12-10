#include "catch_wrapper.hpp"
#include "symbolic.hpp"

SCENARIO("Symbolic", "[symbolic][non-gui]") {
  GIVEN("5+5: no vars, no constants") {
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
  GIVEN("3*x + 7*x: one var, no constants") {
    std::string expr = "3*x + 7 * x";
    REQUIRE_THROWS_WITH(symbolic::Symbolic(expr, {}, {}), "Unknown symbol: x");
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
  GIVEN("3*x + 7*x, 4*x - 3: two expressions, one var, no constants") {
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
  GIVEN("1.324*x + 2*3: one var, no constants") {
    std::string expr = "1.324 * x + 2*3";
    symbolic::Symbolic sym(expr, {"x"}, {});
    CAPTURE(expr);
    REQUIRE(sym.simplify() == "6 + 1.324*x");
    REQUIRE(sym.diff("x") == "1.324");
    std::vector<double> res(1, 0);
    sym.eval(res, {0.1});
    REQUIRE(res[0] == dbl_approx(6.1324));
  }
  GIVEN("3*x + 4/x - 1.0*x + 0.2*x*x - 0.1: one var, no constants") {
    std::string expr = "3*x + 4/x - 1.0*x + 0.2*x*x - 0.1";
    symbolic::Symbolic sym(expr, {"x"}, {});
    CAPTURE(expr);
    REQUIRE(sym.simplify() == "-0.1 + 2.0*x + 4*x^(-1) + 0.2*x^2");
    REQUIRE(sym.diff("x") == "2.0 + 0.4*x - 4*x^(-2)");
    std::vector<double> res(1, 0);
    for (auto x : std::vector<double>{-0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
      sym.eval(res, {x});
      REQUIRE(res[0] ==
              dbl_approx(3 * x + 4 / x - 1.0 * x + 0.2 * x * x - 0.1));
    }
  }
  GIVEN("3*x + 4/y - 1.0*x + 0.2*x*y - 0.1: two vars, no constants") {
    std::string expr = "3*x + 4/y - 1.0*x + 0.2*x*y - 0.1";
    REQUIRE_THROWS_WITH(symbolic::Symbolic(expr, {"x"}, {}),
                        "Unknown symbol: y");
    REQUIRE_THROWS_WITH(symbolic::Symbolic(expr, {"y"}, {}),
                        "Unknown symbol: x");
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
  GIVEN("two expressions, three vars, no constants") {
    std::vector<std::string> expr = {"3*x + 4/y - 1.0*x + 0.2*x*y - 0.1",
                                     "z - cos(x)*sin(y) - x*y"};
    REQUIRE_THROWS_WITH(symbolic::Symbolic(expr, {"z", "x"}, {}),
                        "Unknown symbol: y");
    REQUIRE_THROWS_WITH(symbolic::Symbolic(expr, {"x", "y"}, {}),
                        "Unknown symbol: z");
    REQUIRE_THROWS_WITH(symbolic::Symbolic(expr, {"z", "y"}, {}),
                        "Unknown symbol: x");
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
  GIVEN("e^(4*x): print exponential function") {
    std::string expr = "e^(4*x)";
    REQUIRE_THROWS_WITH(symbolic::Symbolic(expr, {}, {}), "Unknown symbol: x");
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
  GIVEN("x^(3/2): print square-root function") {
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
  GIVEN("3*x + alpha*x - a*n/x: one var, constants") {
    std::map<std::string, double> constants;
    constants["alpha"] = 0.5;
    constants["a"] = 0.8 + 1e-11;
    constants["n"] = -23;
    constants["Unused"] = -99;
    std::string expr = "3*x + alpha*x - a*n*x";
    REQUIRE_THROWS_WITH(symbolic::Symbolic(expr, {}, constants),
                        "Unknown symbol: x");
    REQUIRE_THROWS_WITH(symbolic::Symbolic(expr, {"x"}, {{"a", 1}, {"n", 2}}),
                        "Unknown symbol: alpha");
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
  GIVEN("expression: parse without compiling, then compile") {
    std::string expr = "1.324 * x + 2*3";
    symbolic::Symbolic sym(expr, {"x"}, {}, false);
    CAPTURE(expr);
    REQUIRE(sym.simplify() == "6 + 1.324*x");
    REQUIRE(sym.diff("x") == "1.324");
    std::vector<double> res(1, 0.0);
    sym.compile();
    sym.evalLLVM(res, {0.1});
    REQUIRE(res[0] == dbl_approx(6.1324));
    sym.compile();
    sym.evalLLVM(res, {0.1});
    REQUIRE(res[0] == dbl_approx(6.1324));
  }

  GIVEN("relabel one expression with two vars") {
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
  GIVEN("invalid variable relabeling is a no-op") {
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
  GIVEN("divide expression with number") {
    REQUIRE(symbolic::divide("x", "1.3") == "x/1.3");
    REQUIRE(symbolic::divide("1", "2") == "2^(-1)");
    REQUIRE(symbolic::divide("10*x", "5") == "10*x/5");
    REQUIRE(symbolic::divide("(cos(x))^2+3", "3.14") == "(3 + cos(x)^2)/3.14");
    REQUIRE(symbolic::divide("2*unknown_function(a,b,c)", "2") ==
            "2*unknown_function(a, b, c)/2");
  }
  GIVEN("divide expression with symbol") {
    REQUIRE(symbolic::divide("x", "x") == "1");
    REQUIRE(symbolic::divide("1", "x") == "x^(-1)");
    REQUIRE(symbolic::divide("0", "x") == "0");
    REQUIRE(symbolic::divide("x^2", "x") == "x");
    REQUIRE(symbolic::divide("x+3*x", "x") == "4");
  }
  GIVEN("divide expression with other symbols") {
    REQUIRE(symbolic::divide("x+a", "x") == "(a + x)/x");
    REQUIRE(symbolic::divide("x+y", "x") == "(x + y)/x");
    REQUIRE(symbolic::divide("y*x", "x") == "y");
    REQUIRE(symbolic::divide("y*x*z/y", "x") == "z");
  }
  GIVEN("divide expression with other symbols & functions") {
    REQUIRE(symbolic::divide("sin(x+a)", "x") == "sin(a + x)/x");
    REQUIRE(symbolic::divide("x*sin(x+a)", "x") == "sin(a + x)");
    REQUIRE(symbolic::divide("x*unknown(z)", "x") == "unknown(z)");
    REQUIRE(symbolic::divide("x*(unknown1(unknown2(x)) + 2*another_unknown(q))",
                             "x") ==
            "2*another_unknown(q) + unknown1(unknown2(x))");
    REQUIRE(symbolic::divide("unknown1(unknown2(z))", "x") ==
            "unknown1(unknown2(z))/x");
  }
}
