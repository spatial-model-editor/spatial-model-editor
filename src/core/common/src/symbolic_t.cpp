#include "catch_wrapper.hpp"
#include "math_test_utils.hpp"
#include "symbolic.hpp"
#include <cmath>

using namespace sme;

SCENARIO("Symbolic", "[core/common/symbolic][core/common][core][symbolic]") {
  GIVEN("5+5: no vars, no constants") {
    std::string expr = "5+5";
    utils::Symbolic sym(expr);
    CAPTURE(expr);
    REQUIRE(sym.expr() == "10");
    REQUIRE(sym.inlinedExpr() == "10");
    std::vector<double> res(1, 0);
    sym.eval(res);
    REQUIRE(res[0] == dbl_approx(10));
    sym.rescale(1.0);
    sym.eval(res);
    REQUIRE(res[0] == dbl_approx(10));
  }
  GIVEN("3*x + 7*x: one var, no constants") {
    std::string expr = "3*x + 7 * x";
    auto sym = utils::Symbolic(expr);
    REQUIRE(!sym.isValid());
    REQUIRE(sym.getErrorMessage() == "Unknown symbol: x");
    sym = utils::Symbolic(expr, {"x"});
    CAPTURE(expr);
    REQUIRE(sym.expr() == "10*x");
    REQUIRE(sym.inlinedExpr() == "10*x");
    REQUIRE(sym.diff("x") == "10");
    std::vector<double> res(1, 0);
    sym.eval(res, {0.2});
    REQUIRE(res[0] == dbl_approx(2));
    sym.eval(res, {-1e-22});
    REQUIRE(res[0] == dbl_approx(-1e-21));
    sym.rescale(2.0); // multiply all vars by 2
    sym.eval(res, {0.2});
    REQUIRE(res[0] == dbl_approx(4));
    sym.rescale(2.0, {"x", "y"}); // multiply all vars except x or y by 2
    sym.eval(res, {0.2});
    REQUIRE(res[0] == dbl_approx(4));
  }
  GIVEN("3*x + 7*x, 4*x - 3: two expressions, one var, no constants") {
    std::vector<std::string> expr{"3*x + 7 * x", "4*x - 3"};
    utils::Symbolic sym(expr, {"x"}, {});
    CAPTURE(expr);
    REQUIRE(sym.inlinedExpr(0) == "10*x");
    REQUIRE(symEq(sym.inlinedExpr(1), "4*x - 3"));
    REQUIRE(sym.diff("x", 0) == "10");
    REQUIRE(sym.diff("x", 1) == "4");
    std::vector<double> res(2, 0);
    sym.eval(res, {0.2});
    REQUIRE(res[0] == dbl_approx(2));
    REQUIRE(res[1] == dbl_approx(-2.2));
    sym.eval(res, {-1e-22});
    REQUIRE(res[0] == dbl_approx(-1e-21));
    REQUIRE(res[1] == dbl_approx(-4e-22 - 3));
    sym.rescale(2.0); // multiply all vars by 2
    sym.eval(res, {0.2});
    REQUIRE(res[0] == dbl_approx(4));
    REQUIRE(res[1] == dbl_approx(-1.4));
    sym.rescale(0.25); // multiply all vars by 0.25
    sym.eval(res, {0.2});
    REQUIRE(res[0] == dbl_approx(1));
    REQUIRE(res[1] == dbl_approx(-2.6));
    sym.rescale(0); // multiply all vars by 0
    sym.eval(res, {0.93252});
    REQUIRE(res[0] == dbl_approx(0));
    REQUIRE(res[1] == dbl_approx(-3));
  }
  GIVEN("1.324*x + 2*3: one var, no constants") {
    std::string expr = "1.324 * x + 2*3";
    utils::Symbolic sym(expr, {"x"}, {});
    CAPTURE(expr);
    REQUIRE(symEq(sym.inlinedExpr(), "6 + 1.324*x"));
    REQUIRE(sym.diff("x") == "1.324");
    std::vector<double> res(1, 0);
    sym.eval(res, {0.1});
    REQUIRE(res[0] == dbl_approx(6.1324));
  }
  GIVEN("0.324*x + 2*3: one var, no constants") {
    std::string expr = "0.324 * x + 2*3";
    utils::Symbolic sym(expr, {"x"}, {});
    CAPTURE(expr);
    REQUIRE(symEq(sym.inlinedExpr(), "6 + 0.324*x"));
    REQUIRE(sym.diff("x") == "0.324");
    std::vector<double> res(1, 0);
    sym.eval(res, {0.1});
    REQUIRE(res[0] == dbl_approx(6.0324));
  }
  GIVEN("3*x + 4/x - 1.0*x + 0.2*x*x - 0.1: one var, no constants") {
    std::string expr = "3*x + 4/x - 1.0*x + 0.2*x*x - 0.1";
    utils::Symbolic sym(expr, {"x"}, {});
    CAPTURE(expr);
    REQUIRE(symEq(sym.inlinedExpr(), "-0.1 + 2.0*x + 4*x^(-1) + 0.2*x^2"));
    REQUIRE(symEq(sym.diff("x"), "2.0 + 0.4*x - 4*x^(-2)"));
    std::vector<double> res(1, 0);
    for (auto x : std::vector<double>{-0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
      sym.eval(res, {x});
      REQUIRE(res[0] ==
              dbl_approx(3 * x + 4 / x - 1.0 * x + 0.2 * x * x - 0.1));
    }
  }
  GIVEN("3*x + 4/y - 1.0*x + 0.2*x*y - 0.1: two vars, no constants") {
    std::string expr = "3*x + 4/y - 1.0*x + 0.2*x*y - 0.1";
    REQUIRE(utils::Symbolic(expr, {"x"}, {}).getErrorMessage() ==
            "Unknown symbol: y");
    REQUIRE(utils::Symbolic(expr, {"y"}, {}).getErrorMessage() ==
            "Unknown symbol: x");
    utils::Symbolic sym(expr, {"x", "y", "z"}, {});
    CAPTURE(expr);
    REQUIRE(symEq(sym.inlinedExpr(), "-0.1 + 2.0*x + 0.2*x*y + 4*y^(-1)"));
    REQUIRE(symEq(sym.diff("x"), "2.0 + 0.2*y"));
    REQUIRE(symEq(sym.diff("y"), "0.2*x - 4*y^(-2)"));
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
    REQUIRE(utils::Symbolic(expr, {"z", "x"}, {}).getErrorMessage() ==
            "Unknown symbol: y");
    REQUIRE(utils::Symbolic(expr, {"x", "y"}, {}).getErrorMessage() ==
            "Unknown symbol: z");
    REQUIRE(utils::Symbolic(expr, {"z", "y"}, {}).getErrorMessage() ==
            "Unknown symbol: x");
    utils::Symbolic sym(expr, {"x", "y", "z"}, {});
    CAPTURE(expr);
    REQUIRE(symEq(sym.inlinedExpr(0), "-0.1 + 2.0*x + 0.2*x*y + 4*y^(-1)"));
    REQUIRE(symEq(sym.inlinedExpr(1), "z - x*y - sin(y)*cos(x)"));
    REQUIRE(symEq(sym.diff("x", 0), "2.0 + 0.2*y"));
    REQUIRE(symEq(sym.diff("y", 0), "0.2*x - 4*y^(-2)"));
    REQUIRE(sym.diff("z", 0) == "0");
    REQUIRE(symEq(sym.diff("x", 1), "-y + sin(y)*sin(x)"));
    REQUIRE(symEq(sym.diff("y", 1), "-x - cos(y)*cos(x)"));
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
  GIVEN("exponentiale^(4*x): print exponential function") {
    std::string expr = "exponentiale^(4*x)";
    REQUIRE(utils::Symbolic(expr, {}, {}).getErrorMessage() ==
            "Unknown symbol: x");
    utils::Symbolic sym(expr, {"x"}, {});
    CAPTURE(expr);
    REQUIRE(sym.inlinedExpr() == "exp(4*x)");
    REQUIRE(sym.diff("x") == "4*exp(4*x)");
    std::vector<double> res(1, 0);
    for (auto x : std::vector<double>{-0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
      sym.eval(res, {x});
      REQUIRE(res[0] == dbl_approx(exp(4 * x)));
    }
  }
  GIVEN("x^(3/2): print square-root function") {
    std::string expr = "x^(3/2)";
    utils::Symbolic sym(expr, {"x"}, {});
    CAPTURE(expr);
    REQUIRE(sym.inlinedExpr() == "x^(3/2)");
    REQUIRE(sym.diff("x") == "(3/2)*sqrt(x)");
    std::vector<double> res(1, 0);
    for (auto x : std::vector<double>{0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
      sym.eval(res, {x});
      REQUIRE(res[0] == dbl_approx(x * sqrt(x)));
    }
  }
  GIVEN("3*x + alpha*x - a*n/x: one var, constants") {
    std::vector<std::pair<std::string, double>> constants;
    constants.push_back({"alpha", 0.5});
    constants.push_back({"a", 0.8 + 1e-11});
    constants.push_back({"n", -23});
    constants.push_back({"Unused", -99});
    std::string expr = "3*x + alpha*x - a*n*x";
    REQUIRE(utils::Symbolic(expr, {}, constants).getErrorMessage() ==
            "Unknown symbol: x");
    REQUIRE(
        utils::Symbolic(expr, {"x"}, {{"a", 1}, {"n", 2}}).getErrorMessage() ==
        "Unknown symbol: alpha");
    utils::Symbolic sym(expr, {"x", "y"}, constants);
    CAPTURE(expr);
    REQUIRE(symEq(sym.expr(), "3*x + x*alpha - a*n*x"));
    REQUIRE(sym.inlinedExpr() == "21.90000000023*x");
    REQUIRE(sym.diff("x") == "21.90000000023");
    REQUIRE(sym.diff("y") == "0");
    std::vector<double> res(1, 0);
    for (auto x : std::vector<double>{-0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
      sym.eval(res, {x, 2356.546});
      REQUIRE(res[0] ==
              dbl_approx(3 * x + 0.5 * x - (0.8 + 1e-11) * (-23) * x));
    }
  }
  GIVEN("expression: parse without compiling, then compile") {
    std::string expr = "1.324 * x + 2*3";
    utils::Symbolic sym(expr, {"x"}, {}, {}, false);
    CAPTURE(expr);
    REQUIRE(sym.inlinedExpr() == "6 + 1.324*x");
    REQUIRE(sym.diff("x") == "1.324");
    std::vector<double> res(1, 0.0);
    sym.compile();
    sym.eval(res, {0.1});
    REQUIRE(res[0] == dbl_approx(6.1324));
    sym.compile();
    sym.eval(res, {0.1});
    REQUIRE(res[0] == dbl_approx(6.1324));
  }
  GIVEN("relabel one expression with two vars") {
    std::string expr = "3*x + 12*sin(y)";
    utils::Symbolic sym(expr, {"x", "y"});
    CAPTURE(expr);
    REQUIRE(sym.diff("x") == "3");
    REQUIRE(sym.diff("y") == "12*cos(y)");
    sym.relabel({"newX", "newY"});
    REQUIRE(sym.inlinedExpr() == "3*newX + 12*sin(newY)");
    REQUIRE(sym.diff("newX") == "3");
    REQUIRE(sym.diff("newY") == "12*cos(newY)");
  }
  GIVEN("invalid variable relabeling is a no-op") {
    std::string expr = "3*x + 12*sin(y)";
    utils::Symbolic sym(expr, {"x", "y"});
    CAPTURE(expr);
    REQUIRE(sym.inlinedExpr() == expr);
    REQUIRE(sym.diff("x") == "3");
    REQUIRE(sym.diff("y") == "12*cos(y)");
    sym.relabel({"a"});
    REQUIRE(sym.inlinedExpr() == expr);
    sym.relabel({"a", "b", "c"});
    REQUIRE(sym.inlinedExpr() == expr);
  }
  GIVEN("unknown function") {
    std::string expr = "abcd(y)";
    utils::Symbolic sym(expr, {"y"});
    REQUIRE(!sym.isValid());
    REQUIRE(sym.getErrorMessage() == "Unknown function: abcd(y)");
    CAPTURE(expr);
  }
  GIVEN("unknown functions") {
    std::string expr = "abcd(y) + unknown_function(y)";
    utils::Symbolic sym(expr, {"y"});
    REQUIRE(!sym.isValid());
    REQUIRE(sym.getErrorMessage() == "Unknown function: abcd(y)");
    CAPTURE(expr);
  }
  GIVEN("functions calling unknown functions") {
    std::string expr = "pow(2*y + cos(abcd(y)), 2)";
    utils::Symbolic sym(expr, {"y"});
    REQUIRE(!sym.isValid());
    REQUIRE(sym.getErrorMessage() == "Unknown function: abcd(y)");
    CAPTURE(expr);
  }
  GIVEN("some user-defined functions") {
    utils::Function f1;
    f1.id = "f1";
    f1.name = "my func";
    f1.args = {"x"};
    f1.body = "2*x";
    utils::Function f2;
    f2.id = "f2";
    f2.name = "func2";
    f2.args = {"x", "y"};
    f2.body = "2*x*y";
    utils::Function g2;
    g2.id = "g2";
    g2.name = "g2";
    g2.args = {"a", "b"};
    g2.body = "1/a/b";
    WHEN("1-arg func") {
      std::string expr{"2*f1(z)"};
      utils::Symbolic sym(expr, {"z"}, {}, {f1});
      REQUIRE(sym.isValid());
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(sym.expr() == "2*f1(z)");
      REQUIRE(sym.inlinedExpr() == "4*z");
      CAPTURE(expr);
    }
    WHEN("1-arg func called with two arguments") {
      std::string expr{"2*f1(x, y)"};
      utils::Symbolic sym(expr, {"x", "y"}, {}, {f1});
      REQUIRE(!sym.isValid());
      REQUIRE(sym.getErrorMessage() ==
              "Function 'my func' requires 1 argument(s), found 2");
      CAPTURE(expr);
    }
    WHEN("2-arg func called with one argument") {
      std::string expr{"2*f2(a)"};
      utils::Symbolic sym(expr, {"a"}, {}, {f2});
      REQUIRE(!sym.isValid());
      REQUIRE(sym.getErrorMessage() ==
              "Function 'func2' requires 2 argument(s), found 1");
      CAPTURE(expr);
    }
    WHEN("1-arg func calls itself") {
      std::string expr = "f1(2+f1(f1(2*f1(z))))";
      utils::Symbolic sym(expr, {"z"}, {}, {f1});
      CAPTURE(sym.getErrorMessage());
      REQUIRE(sym.isValid());
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(symEq(sym.inlinedExpr(), "2*(2 + 16*z)"));
      CAPTURE(expr);
    }
    WHEN("1-arg func calls 2-arg func") {
      std::string expr = "f1(1+f2(alpha, beta))";
      utils::Symbolic sym(expr, {"alpha", "beta"}, {}, {f1, f2});
      CAPTURE(sym.getErrorMessage());
      REQUIRE(sym.isValid());
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(symEq(sym.inlinedExpr(), "2*(1+2*alpha*beta)"));
      CAPTURE(expr);
    }
    WHEN("2-arg func calls 2-arg func") {
      std::string expr =
          "f2(x, alpha)*f2(beta, y)*g2(f2(x, y), f2(alpha, beta))";
      utils::Symbolic sym(expr, {"alpha", "beta", "x", "y"}, {}, {f1, f2, g2});
      CAPTURE(sym.getErrorMessage());
      REQUIRE(sym.isValid());
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(symEq(sym.inlinedExpr(), "1"));
      CAPTURE(expr);
    }
  }
  GIVEN("single recursive function call") {
    utils::Function f1;
    f1.id = "f1";
    f1.name = "my func";
    f1.args = {"x"};
    f1.body = "1+f1(x)";
    std::string expr{"f1(a)"};
    utils::Symbolic sym(expr, {"a"}, {}, {f1});
    CAPTURE(sym.getErrorMessage());
    REQUIRE(!sym.isValid());
    REQUIRE(sym.getErrorMessage() == "Recursive function calls not supported");
    CAPTURE(expr);
  }
  GIVEN("symbolicDivide expression with number") {
    REQUIRE(utils::symbolicDivide("x", "1.3") == "x/1.3");
    REQUIRE(utils::symbolicDivide("1", "2") == "2^(-1)");
    REQUIRE(utils::symbolicDivide("10*x", "5") == "10*x/5");
    REQUIRE(utils::symbolicDivide("(cos(x))^2+3", "3.14") ==
            "(3 + cos(x)^2)/3.14");
    REQUIRE(utils::symbolicDivide("2*unknown_function(a,b,c)", "2") ==
            "2*unknown_function(a, b, c)/2");
  }
  GIVEN("symbolicDivide expression with symbol") {
    REQUIRE(utils::symbolicDivide("x", "x") == "1");
    REQUIRE(utils::symbolicDivide("1", "x") == "x^(-1)");
    REQUIRE(utils::symbolicDivide("0", "x") == "0");
    REQUIRE(utils::symbolicDivide("x^2", "x") == "x");
    REQUIRE(utils::symbolicDivide("x+3*x", "x") == "4");
  }
  GIVEN("symbolicDivide expression with other symbols") {
    REQUIRE(utils::symbolicDivide("x+a", "x") == "(a + x)/x");
    REQUIRE(utils::symbolicDivide("x+y", "x") == "(x + y)/x");
    REQUIRE(utils::symbolicDivide("y*x", "x") == "y");
    REQUIRE(utils::symbolicDivide("y*x*z/y", "x") == "z");
  }
  GIVEN("symbolicDivide expression with other symbols & functions") {
    REQUIRE(utils::symbolicDivide("sin(x+a)", "x") == "sin(a + x)/x");
    REQUIRE(utils::symbolicDivide("x*sin(x+a)", "x") == "sin(a + x)");
    REQUIRE(utils::symbolicDivide("x*unknown(z)", "x") == "unknown(z)");
    REQUIRE(utils::symbolicDivide(
                "x*(unknown1(unknown2(x)) + 2*another_unknown(q))", "x") ==
            "2*another_unknown(q) + unknown1(unknown2(x))");
    REQUIRE(utils::symbolicDivide("unknown1(unknown2(z))", "x") ==
            "unknown1(unknown2(z))/x");
  }
  GIVEN("symbolicMultiply expression with number") {
    REQUIRE(utils::symbolicMultiply("x", "1.3") == "x*1.3");
    REQUIRE(utils::symbolicMultiply("1", "2") == "2");
    REQUIRE(utils::symbolicMultiply("10*x", "5") == "10*5*x");
    REQUIRE(utils::symbolicMultiply("4.2/x", "x") == "4.2");
    REQUIRE(utils::symbolicMultiply("cos(y)/x", "x") == "cos(y)");
    REQUIRE(utils::symbolicMultiply("unknown(z)/x", "x") == "unknown(z)");
    REQUIRE(utils::symbolicMultiply("x*unknown_function(a,b,c)/x/x", "x") ==
            "unknown_function(a, b, c)");
  }
  GIVEN("check if expression contains symbol") {
    REQUIRE(utils::symbolicContains("x", "x") == true);
    REQUIRE(utils::symbolicContains("1", "x") == false);
    REQUIRE(utils::symbolicContains("y+x^2", "x") == true);
    REQUIRE(utils::symbolicContains("y+x^2", "y") == true);
    REQUIRE(utils::symbolicContains("y+x^2", "z") == false);
    REQUIRE(utils::symbolicContains("x*unknown(z)", "x") == true);
    REQUIRE(utils::symbolicContains("z*unknown(x)", "x") == true);
    REQUIRE(utils::symbolicContains("z*unknown(y)", "x") == false);
    REQUIRE(utils::symbolicContains("(cos(symbol))^2+3", "symbol") == true);
  }
  GIVEN("expressions equal up to double prec in coefficients test equal") {
    REQUIRE(symEq(QString("0.999999999999"), QString("1")) == false);
    REQUIRE(symEq(QString("0.999999999999999999999"), QString("1")) == true);
    REQUIRE(symEq(QString("1e-3*x+y"),
                  QString("9.9999999999999e-4*x + 1.00000000000001*y")) ==
            false);
    REQUIRE(symEq(QString("1e-3*x+y"),
                  QString("9.999999999999999e-4*x + 1.0000000000000001*y")) ==
            true);
  }
}
