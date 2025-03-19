#include "catch_wrapper.hpp"
#include "dunesim_steadystate.hpp"
#include "model_test_utils.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("DuneSimSteadyState", "[core/simulate/dunesim][core/"
                                "simulate][core][simulate][dunesim][dune]") {
  auto m{getExampleModel(Mod::GrayScott)};
  std::vector<std::string> comps{"compartment"};
  std::vector<std::vector<std::string>> specs{{"U", "V"}};
  double stop_tolerance = 1e-8;
  SECTION("construct") {
    simulate::DuneSimSteadyState duneSim(m, comps, stop_tolerance);
    REQUIRE(duneSim.errorMessage().empty());
    REQUIRE(duneSim.getStopTolerance() == stop_tolerance);
  }
  SECTION("timeout_simulation") {
    simulate::DuneSimSteadyState duneSim(m, comps, stop_tolerance);
    std::size_t steps = duneSim.run(0.2, 50, []() { return false; });
    REQUIRE(duneSim.errorMessage() == "Simulation timeout");
    REQUIRE(duneSim.getCurrentError() > stop_tolerance);
    REQUIRE(steps > 0);
  }
  SECTION("Run_until_convergence") {
    simulate::DuneSimSteadyState duneSim(m, comps, stop_tolerance);
    std::size_t steps = duneSim.run(0.2, 500000, []() { return false; });
    if (not duneSim.errorMessage().empty()) {
      SPDLOG_CRITICAL("error message: {}", duneSim.errorMessage());
    }
    REQUIRE(duneSim.errorMessage().empty());
    REQUIRE(duneSim.getCurrentError() < stop_tolerance);
    REQUIRE(steps > 0);
  }
  SECTION("Callback_is_provided_and_used_to_stop_simulation") {
    simulate::DuneSimSteadyState duneSim(m, comps, stop_tolerance);
    std::size_t steps = duneSim.run(0.2, -1, []() { return true; });
    REQUIRE(duneSim.errorMessage() == "Simulation cancelled");
    REQUIRE(duneSim.getCurrentError() > stop_tolerance);
    REQUIRE(steps > 0);
  }
}
