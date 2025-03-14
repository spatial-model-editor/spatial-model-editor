#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "pixelsim_steadystate.hpp"
#include "sme/model.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("PixelSimSteadyState",
          "[core/simulate/pixelsimsteadystate][core/"
          "simulate][core][simulate][pixelsteadystate]") {
  SECTION("get dc/dt vector") { REQUIRE(3 == 6); }
  SECTION("Run until convergence") { REQUIRE(3 == 6); }
  SECTION("Callback is provided and used to stop simulation") {
    REQUIRE(3 == 6);
  }
}
