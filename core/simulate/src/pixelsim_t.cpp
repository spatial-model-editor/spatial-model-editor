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
  SECTION("Storage scales dcdt for species") {
    auto mDefault{getExampleModel(Mod::ABtoC)};
    auto mStorage{getExampleModel(Mod::ABtoC)};
    mStorage.getSpecies().setStorage("C", 2.0);
    for (auto *m : {&mDefault, &mStorage}) {
      auto &options{m->getSimulationSettings().options};
      options.pixel.enableMultiThreading = false;
      options.pixel.integrator = simulate::PixelIntegratorType::RK101;
      options.pixel.maxTimestep = 1e-3;
    }
    std::vector<std::string> comps{"comp"};
    std::vector<std::vector<std::string>> specs{{"A", "B", "C"}};
    simulate::PixelSim simDefault(mDefault, comps, specs);
    simulate::PixelSim simStorage(mStorage, comps, specs);
    REQUIRE(simDefault.errorMessage().empty());
    REQUIRE(simStorage.errorMessage().empty());
    simDefault.run(1e-3, -1.0, {});
    simStorage.run(1e-3, -1.0, {});
    const auto &dcdtDefault = simDefault.getDcdt(0);
    const auto &dcdtStorage = simStorage.getDcdt(0);
    REQUIRE(dcdtDefault.size() == dcdtStorage.size());
    for (std::size_t i = 0; i < dcdtDefault.size() / 3; ++i) {
      // A and B unchanged as only C has non-unit storage
      REQUIRE(dcdtStorage[i * 3] == dbl_approx(dcdtDefault[i * 3]));
      REQUIRE(dcdtStorage[i * 3 + 1] == dbl_approx(dcdtDefault[i * 3 + 1]));
      REQUIRE(dcdtStorage[i * 3 + 2] ==
              dbl_approx(0.5 * dcdtDefault[i * 3 + 2]));
    }
  }
}
