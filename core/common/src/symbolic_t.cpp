#include "catch_wrapper.hpp"
#include "math_test_utils.hpp"
#include "sme/symbolic.hpp"
#include <cmath>

using namespace sme;
using namespace sme::test;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("Symbolic", "[core/common/symbolic][core/common][core][symbolic]") {
  SECTION("default constructor") {
    common::Symbolic sym{};
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
  }
  SECTION("5+5: no vars, no constants") {
    std::string expr = "5+5";
    common::Symbolic sym(expr);
    CAPTURE(expr);
    REQUIRE(sym.expr() == "10");
    REQUIRE(sym.inlinedExpr() == "10");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.compile();
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
    std::vector<double> res(1, 0);
    sym.eval(res);
    REQUIRE(res[0] == dbl_approx(10));
    sym.rescale(1.0);
    sym.eval(res);
    REQUIRE(res[0] == dbl_approx(10));
  }
  SECTION("3*x + 7*x: one var, no constants") {
    std::string expr{"3*x + 7 * x"};
    // by default missing variable is an error:
    common::Symbolic sym(expr);
    REQUIRE(!sym.isValid());
    REQUIRE_THAT(sym.getErrorMessage(),
                 ContainsSubstring("Unknown symbol 'x'"));
    // can optionally allow missing variables in the expression:
    sym.parse(expr, {"x"}, {}, {}, true);
    REQUIRE(sym.isValid());
    REQUIRE(sym.getErrorMessage().empty());
    REQUIRE(sym.expr() == "10*x");
    REQUIRE(sym.inlinedExpr() == "10*x");
    // specifying variable allow compiling and differentiating
    sym.parse(expr, {"x"});
    CAPTURE(expr);
    REQUIRE(sym.expr() == "10*x");
    REQUIRE(sym.inlinedExpr() == "10*x");
    REQUIRE(sym.diff("x") == "10");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.compile();
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
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
  SECTION("3*x + 7*x, 4*x - 3: two expressions, one var, no constants") {
    std::vector<std::string> expr{"3*x + 7 * x", "4*x - 3"};
    common::Symbolic sym(expr, {"x"});
    CAPTURE(expr);
    REQUIRE(sym.inlinedExpr(0) == "10*x");
    REQUIRE(symEq(sym.inlinedExpr(1), "4*x - 3"));
    REQUIRE(sym.diff("x", 0) == "10");
    REQUIRE(sym.diff("x", 1) == "4");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.compile();
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
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
  SECTION("1.324*x + 2*3: one var, no constants") {
    std::string expr{"1.324 * x + 2*3"};
    common::Symbolic sym(expr, {"x"}, {});
    CAPTURE(expr);
    REQUIRE(symEq(sym.inlinedExpr(), "6 + 1.324*x"));
    REQUIRE(sym.diff("x") == "1.324");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.rescale(2.00); // multiply all vars by 2
    REQUIRE(symEq(sym.inlinedExpr(), "6 + 2.648*x"));
    REQUIRE(sym.diff("x") == "2.648");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.compile();
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
    std::vector<double> res(1, 0);
    sym.eval(res, {0.05});
    REQUIRE(res[0] == dbl_approx(6.1324));
  }
  SECTION("0.324*x + 2*3: one var, no constants") {
    std::string expr{"0.324 * x + 2*3"};
    common::Symbolic sym(expr, {"x"}, {});
    CAPTURE(expr);
    REQUIRE(symEq(sym.inlinedExpr(), "6 + 0.324*x"));
    REQUIRE(sym.diff("x") == "0.324");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.compile();
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
    std::vector<double> res(1, 0);
    sym.eval(res, {0.1});
    REQUIRE(res[0] == dbl_approx(6.0324));
  }
  SECTION("3*x + 4/x - 1.0*x + 0.2*x*x - 0.1: one var, no constants") {
    std::string expr{"3*x + 4/x - 1.0*x + 0.2*x*x - 0.1"};
    common::Symbolic sym(expr, {"x"}, {});
    CAPTURE(expr);
    REQUIRE(symEq(sym.inlinedExpr(), "-0.1 + 2.0*x + 4*x^(-1) + 0.2*x^2"));
    REQUIRE(symEq(sym.diff("x"), "2.0 + 0.4*x - 4*x^(-2)"));
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.compile();
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
    std::vector<double> res(1, 0);
    for (auto x : std::vector<double>{-0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
      sym.eval(res, {x});
      REQUIRE(res[0] ==
              dbl_approx(3 * x + 4 / x - 1.0 * x + 0.2 * x * x - 0.1));
    }
  }
  SECTION("3*x + 4/y - 1.0*x + 0.2*x*y - 0.1: two vars, no constants") {
    std::string expr{"3*x + 4/y - 1.0*x + 0.2*x*y - 0.1"};
    REQUIRE_THAT(common::Symbolic(expr, {"x"}, {}).getErrorMessage(),
                 ContainsSubstring("Unknown symbol 'y'"));
    REQUIRE_THAT(common::Symbolic(expr, {"y"}, {}).getErrorMessage(),
                 ContainsSubstring("Unknown symbol 'x'"));
    common::Symbolic sym(expr, {"x", "y", "z"}, {});
    CAPTURE(expr);
    REQUIRE(symEq(sym.inlinedExpr(), "-0.1 + 2.0*x + 0.2*x*y + 4*y^(-1)"));
    REQUIRE(symEq(sym.diff("x"), "2.0 + 0.2*y"));
    REQUIRE(symEq(sym.diff("y"), "0.2*x - 4*y^(-2)"));
    REQUIRE(sym.diff("z") == "0");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.compile();
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
    std::vector<double> res(1, 0);
    for (auto x : std::vector<double>{-0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
      for (auto y : std::vector<double>{-0.1, 3, 0.9, 23e-17, 4.88e11, 7e32}) {
        sym.eval(res, {x, y});
        REQUIRE(res[0] ==
                dbl_approx(3 * x + 4 / y - 1.0 * x + 0.2 * x * y - 0.1));
      }
    }
  }
  SECTION("two expressions, three vars, no constants") {
    std::vector<std::string> expr{"3*x + 4/y - 1.0*x + 0.2*x*y - 0.1",
                                  "z - cos(x)*sin(y) - x*y"};
    REQUIRE_THAT(common::Symbolic(expr, {"z", "x"}, {}).getErrorMessage(),
                 ContainsSubstring("Unknown symbol 'y'"));
    REQUIRE_THAT(common::Symbolic(expr, {"x", "y"}, {}).getErrorMessage(),
                 ContainsSubstring("Unknown symbol 'z'"));
    REQUIRE_THAT(common::Symbolic(expr, {"z", "y"}, {}).getErrorMessage(),
                 ContainsSubstring("Unknown symbol 'x'"));
    common::Symbolic sym(expr, {"x", "y", "z"}, {});
    CAPTURE(expr);
    REQUIRE(symEq(sym.inlinedExpr(0), "-0.1 + 2.0*x + 0.2*x*y + 4*y^(-1)"));
    REQUIRE(symEq(sym.inlinedExpr(1), "z - x*y - sin(y)*cos(x)"));
    REQUIRE(symEq(sym.diff("x", 0), "2.0 + 0.2*y"));
    REQUIRE(symEq(sym.diff("y", 0), "0.2*x - 4*y^(-2)"));
    REQUIRE(sym.diff("z", 0) == "0");
    REQUIRE(symEq(sym.diff("x", 1), "-y + sin(y)*sin(x)"));
    REQUIRE(symEq(sym.diff("y", 1), "-x - cos(y)*cos(x)"));
    REQUIRE(sym.diff("z", 1) == "1");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.compile();
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
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
  SECTION("exponentiale^(4*x): print exponential function") {
    std::string expr{"exponentiale^(4*x)"};
    REQUIRE_THAT(common::Symbolic(expr, {}, {}).getErrorMessage(),
                 ContainsSubstring("Unknown symbol 'x'"));
    common::Symbolic sym(expr, {"x"}, {});
    CAPTURE(expr);
    REQUIRE(sym.inlinedExpr() == "exp(4*x)");
    REQUIRE(sym.diff("x") == "4*exp(4*x)");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.compile();
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
    std::vector<double> res(1, 0);
    for (auto x : std::vector<double>{-0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
      sym.eval(res, {x});
      REQUIRE(res[0] == dbl_approx(exp(4 * x)));
    }
  }
  SECTION("x^(3/2): print square-root function") {
    std::string expr{"x^(3/2)"};
    common::Symbolic sym(expr, {"x"}, {});
    CAPTURE(expr);
    REQUIRE(sym.inlinedExpr() == "x^(3/2)");
    REQUIRE(sym.diff("x") == "(3/2)*sqrt(x)");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.compile();
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
    std::vector<double> res(1, 0);
    for (auto x : std::vector<double>{0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
      sym.eval(res, {x});
      REQUIRE(res[0] == dbl_approx(x * sqrt(x)));
    }
  }
  SECTION("3*x + alpha*x - a*n/x: one var, constants") {
    std::vector<std::pair<std::string, double>> constants;
    constants.emplace_back("alpha", 0.5);
    constants.emplace_back("a", 0.8 + 1e-11);
    constants.emplace_back("n", -23);
    constants.emplace_back("Unused", -99);
    std::string expr{"3*x + alpha*x - a*n*x"};
    REQUIRE_THAT(common::Symbolic(expr, {}, constants).getErrorMessage(),
                 ContainsSubstring("Unknown symbol 'x'"));
    REQUIRE_THAT(
        common::Symbolic(expr, {"x"}, {{"a", 1}, {"n", 2}}).getErrorMessage(),
        ContainsSubstring("Unknown symbol 'alpha'"));
    common::Symbolic sym(expr, {"x", "y"}, constants);
    CAPTURE(expr);
    REQUIRE(symEq(sym.expr(), "3*x + x*alpha - a*n*x"));
    REQUIRE(sym.inlinedExpr() == "21.90000000023*x");
    REQUIRE(sym.diff("x") == "21.90000000023");
    REQUIRE(sym.diff("y") == "0");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.compile();
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
    std::vector<double> res(1, 0);
    for (auto x : std::vector<double>{-0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
      sym.eval(res, {x, 2356.546});
      REQUIRE(res[0] ==
              dbl_approx(3 * x + 0.5 * x - (0.8 + 1e-11) * (-23) * x));
    }
  }
  SECTION("relabel one expression with two vars") {
    std::string expr{"3*x + 12*sin(y)"};
    common::Symbolic sym(expr, {"x", "y"});
    CAPTURE(expr);
    REQUIRE(sym.diff("x") == "3");
    REQUIRE(sym.diff("y") == "12*cos(y)");
    sym.relabel({"newX", "newY"});
    REQUIRE(sym.inlinedExpr() == "3*newX + 12*sin(newY)");
    REQUIRE(sym.diff("newX") == "3");
    REQUIRE(sym.diff("newY") == "12*cos(newY)");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
  }
  SECTION("relabel one compiled expression with two vars") {
    std::string expr{"3*x + 12*sin(y)"};
    common::Symbolic sym(expr, {"x", "y"});
    sym.compile();
    CAPTURE(expr);
    REQUIRE(sym.diff("x") == "3");
    REQUIRE(sym.diff("y") == "12*cos(y)");
    sym.relabel({"newX", "newY"});
    REQUIRE(sym.inlinedExpr() == "3*newX + 12*sin(newY)");
    REQUIRE(sym.diff("newX") == "3");
    REQUIRE(sym.diff("newY") == "12*cos(newY)");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
  }
  SECTION("invalid variable relabeling is a no-op") {
    std::string expr{"3*x + 12*sin(y)"};
    common::Symbolic sym(expr, {"x", "y"});
    CAPTURE(expr);
    REQUIRE(sym.inlinedExpr() == expr);
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    REQUIRE(sym.diff("x") == "3");
    REQUIRE(sym.diff("y") == "12*cos(y)");
    sym.relabel({"a"});
    REQUIRE(sym.inlinedExpr() == expr);
    sym.relabel({"a", "b", "c"});
    REQUIRE(sym.inlinedExpr() == expr);
  }
  SECTION("unknown function") {
    std::string expr{"abcd(y)"};
    common::Symbolic sym(expr, {"y"});
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
    REQUIRE_THAT(sym.getErrorMessage(),
                 ContainsSubstring("Unknown function 'abcd(y)'"));
    sym.compile();
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
    CAPTURE(expr);
  }
  SECTION("unknown functions") {
    std::string expr{"abcd(y) + unknown_function(y)"};
    common::Symbolic sym(expr, {"y"});
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
    REQUIRE_THAT(sym.getErrorMessage(),
                 ContainsSubstring("Unknown function 'abcd(y)'"));
    sym.compile();
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
    CAPTURE(expr);
  }
  SECTION("functions calling unknown functions") {
    std::string expr{"pow(2*y + cos(abcd(y)), 2)"};
    common::Symbolic sym(expr, {"y"});
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
    REQUIRE_THAT(sym.getErrorMessage(),
                 ContainsSubstring("Unknown function 'abcd(y)'"));
    sym.compile();
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
    CAPTURE(expr);
  }
  SECTION("some user-defined functions") {
    common::SymbolicFunction f0;
    f0.id = "f0";
    f0.name = "zero_arg_func";
    f0.args = {};
    f0.body = "6";
    common::SymbolicFunction f1;
    f1.id = "f1";
    f1.name = "my func";
    f1.args = {"x"};
    f1.body = "2*x";
    common::SymbolicFunction f2;
    f2.id = "f2";
    f2.name = "func2";
    f2.args = {"x", "y"};
    f2.body = "2*x*y";
    common::SymbolicFunction f3;
    f3.id = "f3";
    f3.name = "func3";
    f3.args = {"x", "y"};
    f3.body = "x-y";
    common::SymbolicFunction g2;
    g2.id = "g2";
    g2.name = "g2";
    g2.args = {"a", "b"};
    g2.body = "1/a/b";
    common::SymbolicFunction nested;
    nested.id = "nested";
    nested.name = "nested";
    nested.args = {"a", "b", "c", "d", "w"};
    nested.body = "(3/2)*(a/b)*c*d*f2(f1(f1(w)),g2(f0(),f2(c,d)))";
    SECTION("0-arg func") {
      std::string expr{"z*f0()"};
      common::Symbolic sym(expr, {"z"}, {}, {f0});
      REQUIRE(sym.isValid());
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(sym.expr() == "z*f0()");
      REQUIRE(sym.inlinedExpr() == "6*z");
      CAPTURE(expr);
    }
    SECTION("1-arg func") {
      std::string expr{"2*f1(z)"};
      common::Symbolic sym(expr, {"z"}, {}, {f1});
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(sym.expr() == "2*f1(z)");
      REQUIRE(sym.inlinedExpr() == "4*z");
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == false);
      sym.compile();
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == true);
      CAPTURE(expr);
    }
    SECTION("0-arg func called with one argument") {
      std::string expr{"2*f0(x)"};
      common::Symbolic sym(expr, {"x"}, {}, {f0});
      REQUIRE(!sym.isValid());
      REQUIRE_THAT(
          sym.getErrorMessage(),
          ContainsSubstring(
              "Function 'zero_arg_func' requires 0 argument(s), found 1"));
      CAPTURE(expr);
    }
    SECTION("1-arg func called with two arguments") {
      std::string expr{"2*f1(x, y)"};
      common::Symbolic sym(expr, {"x", "y"}, {}, {f1});
      REQUIRE(sym.isValid() == false);
      REQUIRE(sym.isCompiled() == false);
      REQUIRE_THAT(sym.getErrorMessage(),
                   ContainsSubstring(
                       "Function 'my func' requires 1 argument(s), found 2"));
      CAPTURE(expr);
    }
    SECTION("2-arg func called with one argument") {
      std::string expr{"2*f2(a)"};
      common::Symbolic sym(expr, {"a"}, {}, {f2});
      REQUIRE(sym.isValid() == false);
      REQUIRE(sym.isCompiled() == false);
      REQUIRE_THAT(sym.getErrorMessage(),
                   ContainsSubstring(
                       "Function 'func2' requires 2 argument(s), found 1"));
      CAPTURE(expr);
    }
    SECTION("1-arg func calls itself") {
      std::string expr = "f1(2+f1(f1(2*f1(z))))";
      common::Symbolic sym(expr, {"z"}, {}, {f1});
      CAPTURE(sym.getErrorMessage());
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(symEq(sym.inlinedExpr(), "2*(2 + 16*z)"));
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == false);
      sym.compile();
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == true);
      CAPTURE(expr);
    }
    SECTION("2-arg func called with args in same order") {
      // ensure we don't have the same bug with functions as libsbml:
      // https://github.com/spatial-model-editor/spatial-model-editor/issues/856#issuecomment-1451919941
      std::string expr = "f3(y,x)";
      common::Symbolic sym(expr, {"x", "y"}, {}, {f3});
      CAPTURE(sym.getErrorMessage());
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(symEq(sym.inlinedExpr(), "y-x"));
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == false);
      sym.compile();
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == true);
      CAPTURE(expr);
    }
    SECTION("1-arg func called multiple times with different and same args") {
      std::string expr = "-f1(f1(0.6)) + f1(1.2) - f1(x) * f1(y*z) / "
                         "sqrt(f1(1.20)+f1(1.2)+f1(x))";
      common::Symbolic sym(expr, {"x", "z"},
                           {{"x", 0.1234235}, {"y", 0.71241234}}, {f1});
      CAPTURE(sym.getErrorMessage());
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(symEq(sym.inlinedExpr(), "-0.156559423399418*z"));
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == false);
      sym.compile();
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == true);
      CAPTURE(expr);
    }
    SECTION("1-arg func calls 2-arg func") {
      std::string expr = "f1(1+f2(alpha, beta))";
      common::Symbolic sym(expr, {"alpha", "beta"}, {}, {f1, f2});
      CAPTURE(sym.getErrorMessage());
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(symEq(sym.inlinedExpr(), "2*(1+2*alpha*beta)"));
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == false);
      sym.compile();
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == true);
      CAPTURE(expr);
    }
    SECTION("2-arg func calls 2-arg func") {
      std::string expr =
          "f2(x, alpha)*f2(beta, y)*g2(f2(x, y), f2(alpha, beta))";
      common::Symbolic sym(expr, {"alpha", "beta", "x", "y"}, {}, {f1, f2, g2});
      CAPTURE(sym.getErrorMessage());
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(symEq(sym.inlinedExpr(), "1"));
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == false);
      sym.compile();
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == true);
      CAPTURE(expr);
    }
    SECTION("nested 2-arg functions") {
      std::string expr = "f1(g2(f2(a,b),4))";
      common::Symbolic sym(expr, {"a", "b"}, {}, {f1, f2, g2});
      CAPTURE(sym.getErrorMessage());
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(symEq(sym.inlinedExpr(), "1/(4*a*b)"));
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == false);
      sym.compile();
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == true);
      CAPTURE(expr);
    }
    SECTION("nested with unused function") {
      std::string expr = "f1(f0()+f1(f2(3,f1(w))))";
      common::Symbolic sym(expr, {"w"}, {}, {f0, f1, f2, g2});
      CAPTURE(sym.getErrorMessage());
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(symEq(sym.inlinedExpr(), "12+48*w"));
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == false);
      sym.compile();
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == true);
      CAPTURE(expr);
    }
    SECTION("nested with many variables, one unused") {
      std::string expr = "(3/2)*(a/b)*c*d*f2(f1(f1(w)),g2(f0(),f2(c,d)))";
      common::Symbolic sym(expr, {"a", "b", "c", "d", "w"}, {},
                           {f0, f1, f2, g2});
      CAPTURE(sym.getErrorMessage());
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(symEq(sym.inlinedExpr(), "a*w/b"));
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == false);
      sym.compile();
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == true);
      CAPTURE(expr);
    }
    SECTION("function that calls other functions with one unused argument") {
      std::string expr = "nested(a,b,c,d,w)";
      common::Symbolic sym(expr, {"a", "b", "c", "d", "w"}, {},
                           {nested, f0, f1, f2, g2});
      CAPTURE(sym.getErrorMessage());
      REQUIRE(sym.getErrorMessage().empty());
      REQUIRE(symEq(sym.inlinedExpr(), "a*w/b"));
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == false);
      sym.compile();
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == true);
      CAPTURE(expr);
    }
  }
  SECTION("single recursive function call") {
    common::SymbolicFunction f1;
    f1.id = "f1";
    f1.name = "my func";
    f1.args = {"x"};
    f1.body = "1+f1(x)";
    std::string expr{"f1(a)"};
    common::Symbolic sym(expr, {"a"}, {}, {f1});
    CAPTURE(sym.getErrorMessage());
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
    REQUIRE_THAT(sym.getErrorMessage(),
                 ContainsSubstring("recursive function"));
    REQUIRE_THAT(sym.getErrorMessage(), ContainsSubstring("f1"));
    CAPTURE(expr);
  }
  SECTION("indirect recursive chain of function calls") {
    common::SymbolicFunction r1;
    r1.id = "r1";
    r1.name = "r1";
    r1.args = {"x"};
    r1.body = "1+r2(2*x)";
    common::SymbolicFunction r2;
    r2.id = "r2";
    r2.name = "r2";
    r2.args = {"y"};
    r2.body = "2+r3(3*y)";
    common::SymbolicFunction r3;
    r3.id = "r3";
    r3.name = "r3";
    r3.args = {"x"};
    r3.body = "2*r1(x/12)";
    std::string expr{"6 - r1(2*a) + sqrt(a)*1.34534"};
    common::Symbolic sym(expr, {"a"}, {}, {r1, r2, r3});
    CAPTURE(sym.getErrorMessage());
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
    REQUIRE_THAT(sym.getErrorMessage(),
                 ContainsSubstring("recursive function"));
    REQUIRE_THAT(sym.getErrorMessage(), ContainsSubstring("r1"));
    CAPTURE(expr);
  }
  SECTION("expressions with relative difference < 1e-13 test equal") {
    REQUIRE(symEq(QString("0.9999999999"), QString("1")) == false);
    REQUIRE(symEq(QString("0.99999999999999"), QString("1")) == true);
    REQUIRE(symEq(QString("9999999999"), QString("10000000000")) == false);
    REQUIRE(symEq(QString("99999999999999"), QString("100000000000000")) ==
            true);
    REQUIRE(symEq(QString("0.999999999999999999999"), QString("1")) == true);
    REQUIRE(symEq(QString("1e-3*x+y"),
                  QString("9.99999999999e-4*x + 1.000000000001*y")) == false);
    REQUIRE(symEq(QString("1e-3*x+y"),
                  QString("9.999999999999999e-4*x + 1.0000000000000001*y")) ==
            true);
  }
  SECTION("expression with 1/0: parses but doesn't compile") {
    common::Symbolic sym("1/0");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.compile();
    // 1/0 evaluates to complex infinity in symengine
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
    CAPTURE(sym.getErrorMessage());
    REQUIRE_THAT(sym.getErrorMessage(),
                 ContainsSubstring(
                     "LLVMDouble can only represent real valued infinity"));
  }
  SECTION("expression with inf: parses and compiles") {
    // real infinity is ok both for parsing & for llvm compilation
    common::Symbolic sym("inf", {}, {}, {});
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    CAPTURE(sym.getErrorMessage());
    REQUIRE(sym.getErrorMessage().empty());
    sym.compile();
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
    CAPTURE(sym.getErrorMessage());
    REQUIRE(sym.getErrorMessage().empty());
  }
  SECTION("expression with 1/0: parses but doesn't compile") {
    // 1/0 gives complex inf which is ok for parsing but not for llvm
    // compilation
    common::Symbolic sym("1/0", {}, {}, {});
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    CAPTURE(sym.getErrorMessage());
    REQUIRE(sym.getErrorMessage().empty());
    sym.compile();
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
    CAPTURE(sym.getErrorMessage());
    REQUIRE_THAT(sym.getErrorMessage(), ContainsSubstring("Error compiling"));
  }
  SECTION("expression with nan: parses and may compile depending on symengine "
          "version") {
    // nan parses ok
    common::Symbolic sym("nan");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.compile();
    // todo: expose symengine version in Symbolic & use this below instead of
    // using compile -> invalid as a proxy for earlier symengine versions
    if (sym.isValid()) {
      // compiling a NaN only works in symengine > v0.9.0
      REQUIRE(sym.isValid() == true);
      REQUIRE(sym.isCompiled() == true);
      std::vector<double> res{0.0};
      CAPTURE(sym.getErrorMessage());
      REQUIRE(sym.getErrorMessage().empty());
      sym.eval(res, {});
      REQUIRE(std::isnan(res[0]));
    } else {
      // for previous symengine versions compilation of NaN fails
      REQUIRE(sym.isCompiled() == false);
      CAPTURE(sym.getErrorMessage());
      REQUIRE_THAT(sym.getErrorMessage(), ContainsSubstring("Error compiling"));
    }
  }
  SECTION("3*x + alpha*x - a*n/x: one var, constants") {
    std::vector<std::pair<std::string, double>> constants;
    constants.emplace_back("alpha", 0.5);
    constants.emplace_back("a", 0.8 + 1e-11);
    constants.emplace_back("n", -23);
    constants.emplace_back("Unused", -99);
    std::string expr{"3*x + alpha*x - a*n*x"};
    REQUIRE_THAT(common::Symbolic(expr, {}, constants).getErrorMessage(),
                 ContainsSubstring("Unknown symbol 'x'"));
    REQUIRE_THAT(
        common::Symbolic(expr, {"x"}, {{"a", 1}, {"n", 2}}).getErrorMessage(),
        ContainsSubstring("Unknown symbol 'alpha'"));
    common::Symbolic sym(expr, {"x", "y"}, constants);
    CAPTURE(expr);
    REQUIRE(symEq(sym.expr(), "3*x + x*alpha - a*n*x"));
    REQUIRE(sym.inlinedExpr() == "21.90000000023*x");
    REQUIRE(sym.diff("x") == "21.90000000023");
    REQUIRE(sym.diff("y") == "0");
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    sym.compile();
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
    std::vector<double> res(1, 0);
    for (auto x : std::vector<double>{-0.1, 0, 0.9, 23e-17, 4.88e11, 7e32}) {
      sym.eval(res, {x, 2356.546});
      REQUIRE(res[0] ==
              dbl_approx(3 * x + 0.5 * x - (0.8 + 1e-11) * (-23) * x));
    }
  }
  SECTION("clear") {
    common::Symbolic sym({"3*x + 7 * x", "4*x - 3", "2*x"}, {"x"});
    sym.compile();
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
    sym.clear();
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
  }
  SECTION("parse") {
    common::Symbolic sym{};
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
    REQUIRE(sym.parse({"3*x + 7 * x", "4*x - 3", "2*x"}, {"x"}) == true);
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    REQUIRE(sym.compile() == true);
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
    REQUIRE(sym.parse("notvalidmath", {"x"}) == false);
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
    REQUIRE(sym.parse("2.5*exp(5*y)", {"y"}) == true);
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == false);
    REQUIRE(sym.compile() == true);
    REQUIRE(sym.isValid() == true);
    REQUIRE(sym.isCompiled() == true);
    sym.clear();
    REQUIRE(sym.isValid() == false);
    REQUIRE(sym.isCompiled() == false);
  }
}
