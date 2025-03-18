#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "pixelsim_steadystate.hpp"
#include "sme/model.hpp"
#include <fmt/core.h>
#include <fmt/ranges.h>

using namespace sme;
using namespace sme::test;

TEST_CASE("PixelSimSteadyState",
          "[core/simulate/pixelsimsteadystate][core/"
          "simulate][core][simulate][pixelsteadystate]") {
  auto m{getExampleModel(Mod::GrayScott)};
  std::vector<std::string> comps{"compartment"};
  std::vector<std::vector<std::string>> specs{{"U", "V"}};
  double meta_dt = 0.2;
  double stop_tolerance = 1e-8;
  SECTION("construct_pixelsim_steadystate") {
    simulate::PixelSimSteadyState pixelSim(m, comps, specs, meta_dt,
                                           stop_tolerance);
    REQUIRE(pixelSim.errorMessage().empty());
    REQUIRE(pixelSim.getMetaDt() == meta_dt);
    REQUIRE(pixelSim.getStopTolerance() == stop_tolerance);
  }
  SECTION("timeout-simulation") {
    simulate::PixelSimSteadyState pixelSim(m, comps, specs, meta_dt,
                                           stop_tolerance);
    double normdc_dt = pixelSim.run(50, []() { return false; });
    REQUIRE(normdc_dt > stop_tolerance);
    REQUIRE(pixelSim.errorMessage() == "Simulation stopped early");
  }
  SECTION("Run until convergence") {
    simulate::PixelSimSteadyState pixelSim(m, comps, specs, meta_dt,
                                           stop_tolerance);
    double normdc_dt = pixelSim.run(500000, []() { return false; });
    if (not pixelSim.errorMessage().empty()) {
      SPDLOG_CRITICAL("error message: {}", pixelSim.errorMessage());
    }
    REQUIRE(pixelSim.errorMessage().empty());
    REQUIRE(normdc_dt < stop_tolerance);
  }
  SECTION("Callback is provided and used to stop simulation") {
    simulate::PixelSimSteadyState pixelSim(m, comps, specs, meta_dt,
                                           stop_tolerance);
    pixelSim.run(-1, []() { return true; });
    REQUIRE(pixelSim.errorMessage() == "Simulation stopped early");
  }
}
