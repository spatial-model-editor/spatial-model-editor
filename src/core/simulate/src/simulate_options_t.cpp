#include "catch_wrapper.hpp"
#include "simulate_options.hpp"

using namespace sme;

TEST_CASE("SimulateOptions",
          "[core/simulate][core/simulate_options][core][simulate_options]") {
  SECTION("parseSimulationTimes: valid inputs") {
    auto t1{simulate::parseSimulationTimes("1", "0.1").value()};
    REQUIRE(t1.size() == 1);
    REQUIRE(t1[0].first == 10);
    REQUIRE(t1[0].second == dbl_approx(0.1));

    // dt is rounded to nearest value that fits an integer number of times into
    // t
    auto t2{simulate::parseSimulationTimes("1;4", "198e-3;0.89").value()};
    REQUIRE(t2.size() == 2);
    REQUIRE(t2[0].first == 5);
    REQUIRE(t2[0].second == dbl_approx(0.2));
    REQUIRE(t2[1].first == 4);
    REQUIRE(t2[1].second == dbl_approx(1.0));

    // absolute value is taken of any negative values
    auto t3{simulate::parseSimulationTimes("-1;0.23", "0.497;-0.01").value()};
    REQUIRE(t3.size() == 2);
    REQUIRE(t3[0].first == 2);
    REQUIRE(t3[0].second == dbl_approx(0.5));
    REQUIRE(t3[1].first == 23);
    REQUIRE(t3[1].second == dbl_approx(0.01));
  }
  SECTION("parseSimulationTimes: invalid inputs") {
    REQUIRE(simulate::parseSimulationTimes("1", "").has_value() == false);
    REQUIRE(simulate::parseSimulationTimes("", "3").has_value() == false);
    REQUIRE(simulate::parseSimulationTimes("1", "e").has_value() == false);
    REQUIRE(simulate::parseSimulationTimes("1", "0.2;0.3").has_value() ==
            false);
    REQUIRE(simulate::parseSimulationTimes("1;31;21", "0.2;5;").has_value() ==
            false);
    REQUIRE(
        simulate::parseSimulationTimes("1:31;21", "0.2;5;0.9").has_value() ==
        false);
  }
}
