#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "pixelsim.hpp"
#include "sme/model.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("PixelSim", "[core/simulate/pixelsim][core/"
                      "simulate][core][simulate][pixel]") {
  SECTION("Callback is provided and used to stop simulation") {
    auto m{getExampleModel(Mod::ABtoC)};
    std::vector<std::string> comps{"comp"};
    std::vector<std::vector<std::string>> specs{{"A", "B", "C"}};
    simulate::PixelSim pixelSim(m, comps, specs);
    REQUIRE(pixelSim.errorMessage().empty());
    pixelSim.run(1, -1, []() { return true; });
    REQUIRE(pixelSim.errorMessage() == "Simulation stopped early");
  }
}
