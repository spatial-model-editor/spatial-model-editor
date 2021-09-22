#include "catch_wrapper.hpp"
#include "simulate_data.hpp"

using namespace sme;

TEST_CASE("SimulateData",
          "[core/simulate/simulate][core/simulate_data][core][simulate_data]") {
  simulate::SimulationData data;
  data.timePoints = {0.0, 1.0};
  data.concentration = {{{1.2, -0.881}, {1.0, -0.1}},
                        {{2.2, -2.881}, {3.0, -3.1}}};
  data.avgMinMax.push_back(
      {{{1.0, 2.0, 3.0}, {0.0, 0.1, 0.2}}, {{1.0, 2.0, 3.0}, {0.0, 0.1, 0.2}}});
  data.avgMinMax.push_back({{{3.0, 4.0, 5.0}, {5.0, 5.1, 6.2}},
                            {{6.0, 12.0, 13.0}, {90.0, 90.1, 90.2}}});
  data.concentrationMax = {{{1.0, -0.1}, {1.2, -2.1}},
                           {{3.0, -3.1}, {4.2, -4.1}}};
  data.concPadding = {0, 4};
  data.xmlModel = "sim model";
  REQUIRE(data.timePoints.size() == 2);
  REQUIRE(data.concentration.size() == 2);
  REQUIRE(data.avgMinMax.size() == 2);
  REQUIRE(data.concentrationMax.size() == 2);
  REQUIRE(data.concPadding.size() == 2);
  SECTION("clear()") {
    data.clear();
    REQUIRE(data.timePoints.empty());
    REQUIRE(data.concentration.empty());
    REQUIRE(data.avgMinMax.empty());
    REQUIRE(data.concentrationMax.empty());
    REQUIRE(data.concPadding.empty());
    REQUIRE(data.xmlModel.empty());
  }
  SECTION("pop_back()") {
    data.pop_back();
    REQUIRE(data.timePoints.size() == 1);
    REQUIRE(data.timePoints.back() == dbl_approx(0.0));
    REQUIRE(data.concentration.size() == 1);
    REQUIRE(data.concentration.back()[0][0] == dbl_approx(1.2));
    REQUIRE(data.concentration.back()[0][1] == dbl_approx(-0.881));
    REQUIRE(data.avgMinMax.size() == 1);
    REQUIRE(data.avgMinMax.back()[0][0].avg == dbl_approx(1.0));
    REQUIRE(data.avgMinMax.back()[0][0].min == dbl_approx(2.0));
    REQUIRE(data.avgMinMax.back()[0][0].max == dbl_approx(3.0));
    REQUIRE(data.concentrationMax.size() == 1);
    REQUIRE(data.concentrationMax.back()[0][0] == dbl_approx(1.0));
    REQUIRE(data.concentrationMax.back()[0][1] == dbl_approx(-0.1));
    REQUIRE(data.concPadding.size() == 1);
    REQUIRE(data.concPadding.back() == 0);
    REQUIRE(data.xmlModel == "sim model");
  }
}
