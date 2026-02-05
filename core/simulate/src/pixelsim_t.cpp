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
  SECTION("Uniform diffusion operator matches spatial array diffusion") {
    auto mUniform{getExampleModel(Mod::SingleCompartmentDiffusion)};
    auto mArray{getExampleModel(Mod::SingleCompartmentDiffusion)};
    for (const auto &speciesId : {"slow", "fast"}) {
      auto *field = mArray.getSpecies().getField(speciesId);
      REQUIRE(field != nullptr);
      field->setDiffusionConstant(field->getDiffusionConstant());
    }
    for (auto *m : {&mUniform, &mArray}) {
      auto &options{m->getSimulationSettings().options};
      options.pixel.enableMultiThreading = false;
      options.pixel.integrator = simulate::PixelIntegratorType::RK101;
      options.pixel.maxTimestep = 1e-3;
    }
    std::vector<std::string> comps{"circle"};
    std::vector<std::vector<std::string>> specs{{"slow", "fast"}};
    simulate::PixelSim simUniform(mUniform, comps, specs);
    simulate::PixelSim simArray(mArray, comps, specs);
    REQUIRE(simUniform.errorMessage().empty());
    REQUIRE(simArray.errorMessage().empty());
    simUniform.run(1e-3, -1.0, {});
    simArray.run(1e-3, -1.0, {});
    const auto &cUniform = simUniform.getConcentrations(0);
    const auto &cArray = simArray.getConcentrations(0);
    REQUIRE(cUniform.size() == cArray.size());
    for (std::size_t i = 0; i < cUniform.size(); ++i) {
      REQUIRE(cUniform[i] == dbl_approx(cArray[i]));
    }
  }
}
