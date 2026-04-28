#include "catch_wrapper.hpp"
#include "sme/feature_eval_quantiles.hpp"
#include <stdexcept>
#include <vector>

using namespace sme;

static double computeQuantile(double q, const std::vector<double> &values) {
  simulate::Quantile quantile(q);
  for (const auto value : values) {
    quantile.addData(value);
  }
  return quantile.compute();
}

TEST_CASE("FeatureEvalQuantiles",
          "[core/simulate/feature_eval_quantiles][core/simulate][core][feature_"
          "eval_quantiles]") {
  SECTION("computes nearest quantiles on unsorted input") {
    const std::vector<double> values = {8.0, 2.0, 4.0, 10.0, 6.0};
    REQUIRE(computeQuantile(0.0, values) == dbl_approx(2.0));
    REQUIRE(computeQuantile(0.25, values) == dbl_approx(4.0));
    REQUIRE(computeQuantile(0.5, values) == dbl_approx(6.0));
    REQUIRE(computeQuantile(0.75, values) == dbl_approx(8.0));
    REQUIRE(computeQuantile(1.0, values) == dbl_approx(10.0));
  }

  SECTION("handles duplicate and negative values") {
    const std::vector<double> values = {-4.0, 3.0, -4.0, 10.0, 3.0};
    REQUIRE(computeQuantile(0.0, values) == dbl_approx(-4.0));
    REQUIRE(computeQuantile(0.25, values) == dbl_approx(-4.0));
    REQUIRE(computeQuantile(0.5, values) == dbl_approx(3.0));
    REQUIRE(computeQuantile(0.75, values) == dbl_approx(3.0));
    REQUIRE(computeQuantile(1.0, values) == dbl_approx(10.0));
  }

  SECTION("clear allows reuse with different data") {
    simulate::Quantile quantile(0.5);
    quantile.addData(10.0);
    quantile.addData(20.0);
    quantile.addData(30.0);
    REQUIRE(quantile.compute() == dbl_approx(20.0));

    quantile.clear();
    quantile.addData(5.0);
    quantile.addData(1.0);
    quantile.addData(9.0);
    REQUIRE(quantile.compute() == dbl_approx(5.0));
  }

  SECTION("set and get quantile value") {
    simulate::Quantile quantile(0.25);
    REQUIRE(quantile.getQuantile() == dbl_approx(0.25));
    quantile.setQuantile(0.75);
    REQUIRE(quantile.getQuantile() == dbl_approx(0.75));

    quantile.addData(8.0);
    quantile.addData(2.0);
    quantile.addData(4.0);
    quantile.addData(10.0);
    quantile.addData(6.0);
    REQUIRE(quantile.compute() == dbl_approx(8.0));
  }

  SECTION("rejects quantile values outside [0, 1]") {
    REQUIRE_THROWS_AS(simulate::Quantile(-0.1), std::invalid_argument);
    REQUIRE_THROWS_AS(simulate::Quantile(1.1), std::invalid_argument);

    simulate::Quantile quantile(0.5);
    REQUIRE_THROWS_AS(quantile.setQuantile(-0.1), std::invalid_argument);
    REQUIRE_THROWS_AS(quantile.setQuantile(1.1), std::invalid_argument);
    REQUIRE(quantile.getQuantile() == dbl_approx(0.5));
  }

  SECTION("input order does not change result") {
    const std::vector<double> values = {9.0, 1.0, 7.0, 3.0, 5.0};
    const std::vector<double> shuffled = {5.0, 9.0, 1.0, 7.0, 3.0};
    REQUIRE(computeQuantile(0.5, shuffled) ==
            dbl_approx(computeQuantile(0.5, values)));
    REQUIRE(computeQuantile(0.75, shuffled) ==
            dbl_approx(computeQuantile(0.75, values)));
  }

  SECTION("empty quantile returns zero") {
    simulate::Quantile quantile(0.5);
    REQUIRE(quantile.compute() == dbl_approx(0.0));
  }
}
