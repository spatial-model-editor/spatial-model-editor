#include "catch.hpp"

#include "numerics.h"

TEST_CASE("evaluate expression: changing vars changes expression, changing constants doesn't"){
    std::vector<double> s { 0.0, 1.0 };
    std::vector<double> c { 0.5, 0.5 };
    numerics::reaction_eval r("x*c0 + y*cd", {"x", "y"}, s, {"c0", "cd"}, c);
    REQUIRE(r() == 0.5);
    s[0] = 1.0;
    REQUIRE(r() == 1.0);
    c[0] = 99.0;
    c[1] = 199.0;
    REQUIRE(r() == 1.0);
    s[1] = -1.0;
    REQUIRE(r() == 0.0);
}
