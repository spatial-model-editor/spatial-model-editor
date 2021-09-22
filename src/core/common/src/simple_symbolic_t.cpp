#include "catch_wrapper.hpp"
#include "math_test_utils.hpp"
#include "simple_symbolic.hpp"
#include <cmath>

using namespace sme;
using namespace sme::test;

TEST_CASE("SimpleSymbolic",
          "[core/common/simple_symbolic][core/common][core][simple_symbolic]") {
  SECTION("divide expression with number") {
    REQUIRE(symEq(common::SimpleSymbolic::divide("x", "1.3"), "x/1.3"));
    REQUIRE(symEq(common::SimpleSymbolic::divide("1", "2"), "2^(-1)"));
    REQUIRE(symEq(common::SimpleSymbolic::divide("10*x", "5"), "2*x"));
    REQUIRE(symEq(common::SimpleSymbolic::divide("(cos(x))^2+3", "3.14"),
                  "(3 + cos(x)^2)/3.14"));
    REQUIRE(
        symEq(common::SimpleSymbolic::divide("2*unknown_function(a,b,c)", "2"),
              "2*unknown_function(a, b, c)/2"));
  }
  SECTION("divide expression with symbol") {
    REQUIRE(symEq(common::SimpleSymbolic::divide("x", "x"), "1"));
    REQUIRE(symEq(common::SimpleSymbolic::divide("1", "x"), "x^(-1)"));
    REQUIRE(symEq(common::SimpleSymbolic::divide("0", "x"), "0"));
    REQUIRE(symEq(common::SimpleSymbolic::divide("x^2", "x"), "x"));
    REQUIRE(symEq(common::SimpleSymbolic::divide("x+3*x", "x"), "4"));
  }
  SECTION("divide expression with other symbols") {
    REQUIRE(symEq(common::SimpleSymbolic::divide("x+a", "x"), "(a + x)/x"));
    REQUIRE(symEq(common::SimpleSymbolic::divide("x+y", "x"), "(x + y)/x"));
    REQUIRE(symEq(common::SimpleSymbolic::divide("y*x", "x"), "y"));
    REQUIRE(symEq(common::SimpleSymbolic::divide("y*x*z/y", "x"), "z"));
  }
  SECTION("divide expression with other symbols & functions") {
    REQUIRE(
        symEq(common::SimpleSymbolic::divide("sin(x+a)", "x"), "sin(a + x)/x"));
    REQUIRE(
        symEq(common::SimpleSymbolic::divide("x*sin(x+a)", "x"), "sin(a + x)"));
    REQUIRE(symEq(common::SimpleSymbolic::divide("x*unknown(z)", "x"),
                  "unknown(z)"));
    REQUIRE(symEq(common::SimpleSymbolic::divide(
                      "x*(unknown1(unknown2(x)) + 2*another_unknown(q))", "x"),
                  "2*another_unknown(q) + unknown1(unknown2(x))"));
    REQUIRE(symEq(common::SimpleSymbolic::divide("unknown1(unknown2(z))", "x"),
                  "unknown1(unknown2(z))/x"));
  }
  SECTION("multiply expression with number") {
    REQUIRE(symEq(common::SimpleSymbolic::multiply("x", "1.3"), "x*1.3"));
    REQUIRE(symEq(common::SimpleSymbolic::multiply("1", "2"), "2"));
    REQUIRE(symEq(common::SimpleSymbolic::multiply("10*x", "5"), "10*5*x"));
    REQUIRE(symEq(common::SimpleSymbolic::multiply("4.2/x", "x"), "4.2"));
    REQUIRE(symEq(common::SimpleSymbolic::multiply("cos(y)/x", "x"), "cos(y)"));
    REQUIRE(symEq(common::SimpleSymbolic::multiply("unknown(z)/x", "x"),
                  "unknown(z)"));
    REQUIRE(symEq(
        common::SimpleSymbolic::multiply("x*unknown_function(a,b,c)/x/x", "x"),
        "unknown_function(a, b, c)"));
  }
  SECTION("check if expression contains symbol") {
    REQUIRE(common::SimpleSymbolic::contains("x", "x") == true);
    REQUIRE(common::SimpleSymbolic::contains("1", "x") == false);
    REQUIRE(common::SimpleSymbolic::contains("y+x^2", "x") == true);
    REQUIRE(common::SimpleSymbolic::contains("y+x^2", "y") == true);
    REQUIRE(common::SimpleSymbolic::contains("y+x^2", "z") == false);
    REQUIRE(common::SimpleSymbolic::contains("x*unknown(z)", "x") == true);
    REQUIRE(common::SimpleSymbolic::contains("z*unknown(x)", "x") == true);
    REQUIRE(common::SimpleSymbolic::contains("z*unknown(y)", "x") == false);
    REQUIRE(common::SimpleSymbolic::contains("(cos(symbol))^2+3", "symbol") ==
            true);
  }
}
