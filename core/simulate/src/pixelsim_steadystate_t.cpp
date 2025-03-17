#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "pixelsim_steadystate.hpp"
#include "sme/model.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("PixelSimSteadyState",
          "[core/simulate/pixelsimsteadystate][core/"
          "simulate][core][simulate][pixelsteadystate]") {
  auto m{getExampleModel(Mod::ABtoC)};
  std::vector<std::string> comps{"comp"};
  std::vector<std::vector<std::string>> specs{{"A", "B", "C"}};
  double meta_dt = 0.2;
  double stop_tolerance = 1e-6;
  SECTION("construct pixelsim_steadystate") {
    simulate::PixelSimSteadyState pixelSim(m, comps, specs, meta_dt,
                                           stop_tolerance);
    REQUIRE(pixelSim.errorMessage().empty());
    REQUIRE(pixelSim.getMetaDt() == meta_dt);
    REQUIRE(pixelSim.getStopTolerance() == stop_tolerance);
    REQUIRE(pixelSim.getOldState().empty());
    REQUIRE(pixelSim.getDcdt().empty());
  }
  SECTION("ensure vector update during simulation") {
    simulate::PixelSimSteadyState pixelSim(m, comps, specs, meta_dt,
                                           stop_tolerance);
    REQUIRE(pixelSim.getDcdt().empty());
    pixelSim.run(6000, []() { return false; });
    REQUIRE(pixelSim.getDcdt().size() == 3);
  }
  SECTION("Run until convergence") {
    simulate::PixelSimSteadyState pixelSim(m, comps, specs, meta_dt,
                                           stop_tolerance);
    pixelSim.run(6000, []() { return false; });
  }
  SECTION("Callback is provided and used to stop simulation") {
    simulate::PixelSimSteadyState pixelSim(m, comps, specs, meta_dt,
                                           stop_tolerance);
    pixelSim.run(-1, []() { return true; });
    REQUIRE(pixelSim.errorMessage() == "Simulation stopped early");
  }
}
