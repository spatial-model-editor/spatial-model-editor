#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "pixelsim.hpp"
#include "sme/model.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("PixelSimSteadyState",
          "[core/simulate/pixelsimsteadystate][core/"
          "simulate][core][simulate][pixelsteadystate]") {
  SECTION("get dc/dt vector") {}
  SECTION("get max dc/dt") {}
  SECTION("Run until convergence") {}
  SECTION("Callback is provided and used to stop simulation") {}
}
