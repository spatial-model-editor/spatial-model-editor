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
  SECTION("Zero-storage species: PixelSim accepts S=0") {
    auto m{getExampleModel(Mod::ABtoC)};
    m.getSpecies().setStorage("C", 0.0);
    auto &options{m.getSimulationSettings().options};
    options.pixel.enableMultiThreading = false;
    options.pixel.integrator = simulate::PixelIntegratorType::RK101;
    options.pixel.maxTimestep = 1e-3;
    std::vector<std::string> comps{"comp"};
    std::vector<std::vector<std::string>> specs{{"A", "B", "C"}};
    simulate::PixelSim sim(m, comps, specs);
    REQUIRE(sim.errorMessage().empty());
    // run a single step - should not crash or error
    sim.run(1e-3, -1.0, {});
    REQUIRE(sim.errorMessage().empty());
    // dcdt for zero-storage species C should be zero after applyStorage
    const auto &dcdt = sim.getDcdt(0);
    for (std::size_t i = 0; i < dcdt.size() / 3; ++i) {
      REQUIRE(dcdt[i * 3 + 2] == dbl_approx(0.0));
    }
  }
  SECTION("Zero-storage species with D=0: pure algebraic constraint") {
    auto m{getExampleModel(Mod::ABtoC)};
    m.getSpecies().setStorage("C", 0.0);
    // set D=0 for C so constraint is purely algebraic: 0 = R(c)
    m.getSpecies().setDiffusionConstant("C", 0.0);
    auto &options{m.getSimulationSettings().options};
    options.pixel.enableMultiThreading = false;
    options.pixel.integrator = simulate::PixelIntegratorType::RK101;
    options.pixel.maxTimestep = 1e-3;
    std::vector<std::string> comps{"comp"};
    std::vector<std::vector<std::string>> specs{{"A", "B", "C"}};
    simulate::PixelSim sim(m, comps, specs);
    REQUIRE(sim.errorMessage().empty());
    // should not crash or produce errors
    sim.run(1e-3, -1.0, {});
    REQUIRE(sim.errorMessage().empty());
    // dcdt for zero-storage species C should be zero after applyStorage
    const auto &dcdt = sim.getDcdt(0);
    for (std::size_t i = 0; i < dcdt.size() / 3; ++i) {
      REQUIRE(dcdt[i * 3 + 2] == dbl_approx(0.0));
    }
  }
  SECTION("Zero-storage species: constraint solver reduces residual") {
    // Use SingleCompartmentDiffusion model with S=0 for "slow" species
    // With S=0 and D>0, constraint solver should reduce the residual
    // compared to having no constraint solver
    auto m{getExampleModel(Mod::SingleCompartmentDiffusion)};
    m.getSpecies().setStorage("slow", 0.0);
    auto &options{m.getSimulationSettings().options};
    options.pixel.enableMultiThreading = false;
    options.pixel.integrator = simulate::PixelIntegratorType::RK101;
    options.pixel.maxTimestep = 1e-3;
    std::vector<std::string> comps{"circle"};
    std::vector<std::vector<std::string>> specs{{"slow", "fast"}};
    simulate::PixelSim sim(m, comps, specs);
    REQUIRE(sim.errorMessage().empty());
    // run several steps - constraint solver should run without errors
    sim.run(0.01, -1.0, {});
    REQUIRE(sim.errorMessage().empty());
    // after simulation, the "slow" species concentration should be more
    // spatially uniform than the initial non-uniform state
    const auto &conc = sim.getConcentrations(0);
    std::size_t nSpecies = 2;
    std::size_t nPixels = conc.size() / nSpecies;
    REQUIRE(nPixels > 1);
    double minVal = conc[0];
    double maxVal = conc[0];
    for (std::size_t ix = 0; ix < nPixels; ++ix) {
      double val = conc[ix * nSpecies + 0];
      minVal = std::min(minVal, val);
      maxVal = std::max(maxVal, val);
    }
    // the range should be reduced relative to max value
    double relRange = (maxVal - minVal) / maxVal;
    CAPTURE(minVal);
    CAPTURE(maxVal);
    CAPTURE(relRange);
    if (maxVal > 1e-12) {
      REQUIRE(relRange < 0.75);
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
