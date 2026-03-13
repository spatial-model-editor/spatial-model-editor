#include "catch_wrapper.hpp"
#include "cudapixelsim.hpp"
#include "model_test_utils.hpp"
#include "sme/model.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("CudaPixelSim unsupported-feature gating",
          "[core/simulate/cudapixelsim][core/simulate][core][simulate]") {
#ifndef SME_HAS_CUDA_PIXEL
  SECTION("Rejects CUDA backend when SME is built without CUDA support") {
    auto m{getExampleModel(Mod::ABtoC)};
    auto &options{m.getSimulationSettings().options};
    options.pixel.backend = simulate::PixelBackendType::CUDA;
    options.pixel.integrator = simulate::PixelIntegratorType::RK101;
    simulate::CudaPixelSim sim(m, {"comp"}, {{"A", "B", "C"}});
    REQUIRE(sim.errorMessage().find("SME_ENABLE_CUDA_PIXEL") !=
            std::string::npos);
  }
#else
  SECTION("Rejects unsupported integrators") {
    auto m{getExampleModel(Mod::ABtoC)};
    auto &options{m.getSimulationSettings().options};
    options.pixel.backend = simulate::PixelBackendType::CUDA;
    options.pixel.integrator = simulate::PixelIntegratorType::RK212;
    simulate::CudaPixelSim sim(m, {"comp"}, {{"A", "B", "C"}});
    REQUIRE(sim.errorMessage().find("RK101") != std::string::npos);
  }

  SECTION("Rejects membrane reactions") {
    auto m{getTestModel("membrane-reaction-pixels")};
    auto &options{m.getSimulationSettings().options};
    options.pixel.backend = simulate::PixelBackendType::CUDA;
    options.pixel.integrator = simulate::PixelIntegratorType::RK101;
    simulate::CudaPixelSim sim(m, {"A", "B"}, {{"x"}, {"y"}});
    REQUIRE(sim.errorMessage().find("membrane reactions") != std::string::npos);
  }

  SECTION("Rejects cross-diffusion") {
    auto m{getExampleModel(Mod::SingleCompartmentDiffusion)};
    m.getSpecies().setCrossDiffusionConstant("slow", "fast", "1");
    auto &options{m.getSimulationSettings().options};
    options.pixel.backend = simulate::PixelBackendType::CUDA;
    options.pixel.integrator = simulate::PixelIntegratorType::RK101;
    simulate::CudaPixelSim sim(m, {"circle"}, {{"slow", "fast"}});
    REQUIRE(sim.errorMessage().find("cross-diffusion") != std::string::npos);
  }

  SECTION("Rejects non-unit storage") {
    auto m{getExampleModel(Mod::ABtoC)};
    m.getSpecies().setStorage("C", 0.5);
    auto &options{m.getSimulationSettings().options};
    options.pixel.backend = simulate::PixelBackendType::CUDA;
    options.pixel.integrator = simulate::PixelIntegratorType::RK101;
    simulate::CudaPixelSim sim(m, {"comp"}, {{"A", "B", "C"}});
    REQUIRE(sim.errorMessage().find("unit storage") != std::string::npos);
  }

  SECTION("Rejects non-uniform diffusion constants") {
    auto m{getExampleModel(Mod::SingleCompartmentDiffusion)};
    auto *field = m.getSpecies().getField("slow");
    REQUIRE(field != nullptr);
    field->setDiffusionConstant(field->getDiffusionConstant());
    auto &options{m.getSimulationSettings().options};
    options.pixel.backend = simulate::PixelBackendType::CUDA;
    options.pixel.integrator = simulate::PixelIntegratorType::RK101;
    simulate::CudaPixelSim sim(m, {"circle"}, {{"slow", "fast"}});
    REQUIRE(sim.errorMessage().find("non-uniform diffusion") !=
            std::string::npos);
  }
#endif
}
