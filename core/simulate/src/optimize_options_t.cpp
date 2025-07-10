#include "catch_wrapper.hpp"
#include "sme/optimize_options.hpp"

using namespace sme;

TEST_CASE("Optimization options",
          "[core/simulate/optimize][core/simulate][core][optimize]") {
  for (const auto &optAlgorithmType : simulate::optAlgorithmTypes) {
    CAPTURE(optAlgorithmType);
    REQUIRE(!simulate::toString(optAlgorithmType).empty());
  }
}
