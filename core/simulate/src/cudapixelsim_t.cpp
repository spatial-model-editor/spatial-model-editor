#include "catch_wrapper.hpp"
#include "cudapixelsim.hpp"
#include "model_test_utils.hpp"
#include "pixelsim.hpp"
#include "sme/model.hpp"
#include "sme/simulate.hpp"
#include "sme/symbolic.hpp"
#include <array>
#include <future>
#include <random>
#include <ranges>
#include <string_view>

using namespace sme;
using namespace sme::test;

namespace {

constexpr double singleStepDt{1e-6};
constexpr double longRunTime{1.0};
constexpr double longRunMaxTimestep{1e-2};
constexpr double comparisonAbsTol{1e-12};
constexpr double comparisonRelTol{1e-10};

constexpr std::array allExampleModels{Mod::ABtoC,
                                      Mod::Brusselator,
                                      Mod::CircadianClock,
                                      Mod::GrayScott,
                                      Mod::GrayScott3D,
                                      Mod::LiverSimplified,
                                      Mod::LiverCells,
                                      Mod::SingleCompartmentDiffusion,
                                      Mod::SingleCompartmentDiffusion3D,
                                      Mod::VerySimpleModel,
                                      Mod::VerySimpleModel3D,
                                      Mod::FitzhughNagumo3D,
                                      Mod::SelKov3D,
                                      Mod::CalciumWavePropagation3D};

constexpr std::array longRunExampleModels{Mod::ABtoC,
                                          Mod::Brusselator,
                                          Mod::CircadianClock,
                                          Mod::GrayScott,
                                          Mod::SingleCompartmentDiffusion,
                                          Mod::VerySimpleModel};

constexpr std::array adaptiveLongRunExampleModels{
    Mod::ABtoC, Mod::SingleCompartmentDiffusion, Mod::VerySimpleModel};

constexpr std::string_view toString(Mod mod) {
  switch (mod) {
  case Mod::ABtoC:
    return "ABtoC";
  case Mod::Brusselator:
    return "Brusselator";
  case Mod::CircadianClock:
    return "CircadianClock";
  case Mod::GrayScott:
    return "GrayScott";
  case Mod::GrayScott3D:
    return "GrayScott3D";
  case Mod::LiverSimplified:
    return "LiverSimplified";
  case Mod::LiverCells:
    return "LiverCells";
  case Mod::SingleCompartmentDiffusion:
    return "SingleCompartmentDiffusion";
  case Mod::SingleCompartmentDiffusion3D:
    return "SingleCompartmentDiffusion3D";
  case Mod::VerySimpleModel:
    return "VerySimpleModel";
  case Mod::VerySimpleModel3D:
    return "VerySimpleModel3D";
  case Mod::FitzhughNagumo3D:
    return "FitzhughNagumo3D";
  case Mod::SelKov3D:
    return "SelKov3D";
  case Mod::CalciumWavePropagation3D:
    return "CalciumWavePropagation3D";
  }
  return "Unknown";
}

void configurePixelBackend(model::Model &m, simulate::PixelBackendType backend,
                           double dt,
                           simulate::PixelIntegratorType integrator =
                               simulate::PixelIntegratorType::RK101) {
  m.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  auto &options{m.getSimulationSettings().options};
  options.pixel.backend = backend;
  options.pixel.integrator = integrator;
  options.pixel.enableMultiThreading = false;
  options.pixel.maxTimestep = dt;
}

std::vector<double> makeRandomConcentrations(std::size_t nPixels,
                                             std::size_t nSpecies,
                                             std::uint32_t seed,
                                             double minValue, double maxValue) {
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> dist(minValue, maxValue);
  std::vector<double> concentrations(nPixels * nSpecies);
  for (auto &c : concentrations) {
    c = dist(rng);
  }
  return concentrations;
}

void setRestartConcentrations(model::Model &m,
                              const std::vector<double> &concentrations) {
  auto &data{m.getSimulationData()};
  data.clear();
  data.timePoints = {0.0, 0.0};
  data.concPadding = {0, 0};
  data.concentration = {std::vector<std::vector<double>>{},
                        std::vector<std::vector<double>>{concentrations}};
}

bool isCudaRuntimeUnavailable(const std::string &msg) {
  constexpr std::array<std::string_view, 6> unavailableMessages{
      "Failed to initialize the CUDA driver",
      "Failed to query CUDA devices",
      "No CUDA devices were found",
      "Failed to get the primary CUDA device",
      "Failed to create the CUDA context",
      "Failed to create the CUDA stream"};
  return std::ranges::any_of(unavailableMessages, [&msg](std::string_view s) {
    return msg.find(s) != std::string::npos;
  });
}

bool requireCudaReady(const simulate::CudaPixelSim &sim) {
  if (sim.errorMessage().empty()) {
    return true;
  }
  if (isCudaRuntimeUnavailable(sim.errorMessage())) {
    WARN("Skipping CUDA numerical comparison: " << sim.errorMessage());
    return false;
  }
  REQUIRE(sim.errorMessage().empty());
  return false;
}

void requireNear(const std::vector<double> &cpuValues,
                 const std::vector<double> &gpuValues) {
  REQUIRE(cpuValues.size() == gpuValues.size());
  for (std::size_t i = 0; i < cpuValues.size(); ++i) {
    CAPTURE(i);
    CAPTURE(cpuValues[i]);
    CAPTURE(gpuValues[i]);
    REQUIRE(gpuValues[i] == Catch::Approx(cpuValues[i])
                                .epsilon(comparisonRelTol)
                                .margin(comparisonAbsTol));
  }
}

void requireMatchingSimulationState(const simulate::Simulation &cpuSim,
                                    const simulate::Simulation &gpuSim,
                                    std::size_t timeIndex) {
  REQUIRE(cpuSim.getCompartmentIds() == gpuSim.getCompartmentIds());
  for (std::size_t iCompartment = 0;
       iCompartment < cpuSim.getCompartmentIds().size(); ++iCompartment) {
    REQUIRE(cpuSim.getSpeciesIds(iCompartment) ==
            gpuSim.getSpeciesIds(iCompartment));
    for (std::size_t iSpecies = 0;
         iSpecies < cpuSim.getSpeciesIds(iCompartment).size(); ++iSpecies) {
      requireNear(cpuSim.getDcdt(iCompartment, iSpecies),
                  gpuSim.getDcdt(iCompartment, iSpecies));
      requireNear(cpuSim.getConc(timeIndex, iCompartment, iSpecies),
                  gpuSim.getConc(timeIndex, iCompartment, iSpecies));
    }
  }
}

void requireMatchingLowerOrderState(const simulate::Simulation &cpuSim,
                                    const simulate::Simulation &gpuSim,
                                    std::size_t timeIndex) {
  REQUIRE(cpuSim.getCompartmentIds() == gpuSim.getCompartmentIds());
  for (std::size_t iCompartment = 0;
       iCompartment < cpuSim.getCompartmentIds().size(); ++iCompartment) {
    REQUIRE(cpuSim.getSpeciesIds(iCompartment) ==
            gpuSim.getSpeciesIds(iCompartment));
    for (std::size_t iSpecies = 0;
         iSpecies < cpuSim.getSpeciesIds(iCompartment).size(); ++iSpecies) {
      const auto cpuConcs = cpuSim.getConc(timeIndex, iCompartment, iSpecies);
      const auto gpuConcs = gpuSim.getConc(timeIndex, iCompartment, iSpecies);
      REQUIRE(cpuConcs.size() == gpuConcs.size());
      for (std::size_t iPixel = 0; iPixel < cpuConcs.size(); ++iPixel) {
        const auto cpuValue =
            cpuSim.getLowerOrderConc(iCompartment, iSpecies, iPixel);
        const auto gpuValue =
            gpuSim.getLowerOrderConc(iCompartment, iSpecies, iPixel);
        CAPTURE(iCompartment);
        CAPTURE(iSpecies);
        CAPTURE(iPixel);
        CAPTURE(cpuValue);
        CAPTURE(gpuValue);
        REQUIRE(gpuValue == Catch::Approx(cpuValue)
                                .epsilon(comparisonRelTol)
                                .margin(comparisonAbsTol));
      }
    }
  }
}

void randomizeReactiveSpecies(model::Model &mCpu, model::Model &mGpu,
                              std::uint32_t seed) {
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> dist(0.05, 1.25);
  for (const auto &compartmentId : mCpu.getCompartments().getIds()) {
    for (const auto &speciesId : mCpu.getSpecies().getIds(compartmentId)) {
      if (mCpu.getSpecies().getIsConstant(speciesId)) {
        continue;
      }
      auto *fieldCpu = mCpu.getSpecies().getField(speciesId);
      auto *fieldGpu = mGpu.getSpecies().getField(speciesId);
      REQUIRE(fieldCpu != nullptr);
      REQUIRE(fieldGpu != nullptr);
      std::vector<double> concentrations(fieldCpu->getConcentration().size());
      for (auto &c : concentrations) {
        c = dist(rng);
      }
      fieldCpu->setConcentration(concentrations);
      fieldGpu->setConcentration(concentrations);
    }
  }
}

} // namespace

TEST_CASE("CudaPixelSim kernel source generation uses Symbolic CUDA code",
          "[core/simulate/cudapixelsim][core/simulate][core][simulate]") {
  const std::string expr{"pi + A"};
  common::Symbolic symbolic(expr, {"A"});
  REQUIRE(symbolic.isValid());
  symbolic.relabel({"sme_var_0"});
  const auto expected = symbolic.cudaCode();
  REQUIRE(expected != expr);

  const auto source = simulate::detail::makeCudaKernelSource({"A"}, {expr});
  REQUIRE(source.find("const double sme_var_0 = sme_values[0];") !=
          std::string::npos);
  REQUIRE(source.find("sme_out[0] = " + expected + ";") != std::string::npos);
}

TEST_CASE("CudaPixelSim kernel source generation uses Symbolic float CUDA code",
          "[core/simulate/cudapixelsim][core/simulate][core][simulate]") {
  const std::string expr{"pi + A"};
  common::Symbolic symbolic(expr, {"A"});
  REQUIRE(symbolic.isValid());
  symbolic.relabel({"sme_var_0"});
  const auto expected = symbolic.cudaCode(0, true);
  REQUIRE(expected != expr);

  const auto source =
      simulate::detail::makeCudaKernelSource({"A"}, {expr}, true);
  REQUIRE(source.find("const float sme_var_0 = sme_values[0];") !=
          std::string::npos);
  REQUIRE(source.find("sme_out[0] = " + expected + ";") != std::string::npos);
  REQUIRE(source.find("float* conc") != std::string::npos);
  REQUIRE(source.find("double* conc") == std::string::npos);
  REQUIRE(source.find("2.0f * c0") != std::string::npos);
}

TEST_CASE("CudaPixelSim kernel source generation avoids variable collisions",
          "[core/simulate/cudapixelsim][core/simulate][core][simulate]") {
  const auto source =
      simulate::detail::makeCudaKernelSource({"c", "h"}, {"c+h"});
  REQUIRE(source.find("const double c = c[0];") == std::string::npos);
  REQUIRE(source.find("const double sme_var_0 = sme_values[0];") !=
          std::string::npos);
  REQUIRE(source.find("const double sme_var_1 = sme_values[1];") !=
          std::string::npos);
}

TEST_CASE("CudaPixelSim NVRTC compile failures include compile log details",
          "[core/simulate/cudapixelsim][core/simulate][core][simulate]") {
  const auto message = simulate::detail::makeNvrtcCompileFailureMessage(
      "Failed to compile CUDA pixel kernel", "NVRTC_ERROR_COMPILATION",
      "test.cu(12): error: identifier \"foo\" is undefined\n");
  REQUIRE(message.find("NVRTC_ERROR_COMPILATION") != std::string::npos);
  REQUIRE(message.find("identifier \"foo\" is undefined") != std::string::npos);
}

TEST_CASE("CudaPixelSim unsupported-feature gating",
          "[core/simulate/cudapixelsim][core/simulate][core][simulate]") {
  SECTION("Rejects cross-diffusion") {
    auto m{getExampleModel(Mod::SingleCompartmentDiffusion)};
    m.getSpecies().setCrossDiffusionConstant("slow", "fast", "1");
    auto &options{m.getSimulationSettings().options};
    options.pixel.backend = simulate::PixelBackendType::GPU;
    options.pixel.integrator = simulate::PixelIntegratorType::RK101;
    simulate::CudaPixelSim sim(m, {"circle"}, {{"slow", "fast"}});
    REQUIRE(sim.errorMessage().find("cross-diffusion") != std::string::npos);
  }

  SECTION("Rejects non-unit storage") {
    auto m{getExampleModel(Mod::ABtoC)};
    m.getSpecies().setStorage("C", 0.5);
    auto &options{m.getSimulationSettings().options};
    options.pixel.backend = simulate::PixelBackendType::GPU;
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
    options.pixel.backend = simulate::PixelBackendType::GPU;
    options.pixel.integrator = simulate::PixelIntegratorType::RK101;
    simulate::CudaPixelSim sim(m, {"circle"}, {{"slow", "fast"}});
    REQUIRE(sim.errorMessage().find("non-uniform diffusion") !=
            std::string::npos);
  }
}

TEST_CASE("CudaPixelSim reaction dcdt matches CPU PixelSim for one step",
          "[core/simulate/cudapixelsim][core/simulate][core][simulate]"
          "[expensive][requires-cuda-gpu]") {
  auto mCpu{getExampleModel(Mod::ABtoC)};
  auto mGpu{getExampleModel(Mod::ABtoC)};
  for (auto *m : {&mCpu, &mGpu}) {
    configurePixelBackend(*m, simulate::PixelBackendType::CPU, singleStepDt);
    for (const auto &speciesId : {"A", "B", "C"}) {
      m->getSpecies().setDiffusionConstant(speciesId, 0.0);
    }
  }
  configurePixelBackend(mGpu, simulate::PixelBackendType::GPU, singleStepDt);

  const auto *field = mCpu.getSpecies().getField("A");
  REQUIRE(field != nullptr);
  const auto inputConcentrations = makeRandomConcentrations(
      field->getConcentration().size(), 3, 12345, 0.05, 1.25);
  setRestartConcentrations(mCpu, inputConcentrations);
  setRestartConcentrations(mGpu, inputConcentrations);

  simulate::PixelSim cpuSim(mCpu, {"comp"}, {{"A", "B", "C"}});
  simulate::CudaPixelSim gpuSim(mGpu, {"comp"}, {{"A", "B", "C"}});
  REQUIRE(cpuSim.errorMessage().empty());
  if (!requireCudaReady(gpuSim)) {
    return;
  }

  const auto cpuSteps = cpuSim.run(singleStepDt, -1.0, {});
  const auto gpuSteps = gpuSim.run(singleStepDt, -1.0, {});
  REQUIRE(cpuSim.errorMessage().empty());
  REQUIRE(gpuSim.errorMessage().empty());
  REQUIRE(cpuSteps == 1);
  REQUIRE(gpuSteps == 1);

  requireNear(cpuSim.getDcdt(0), gpuSim.getDcdt(0));
  requireNear(cpuSim.getConcentrations(0), gpuSim.getConcentrations(0));
}

TEST_CASE("CudaPixelSim diffusion dcdt matches CPU PixelSim for one step",
          "[core/simulate/cudapixelsim][core/simulate][core][simulate]"
          "[expensive][requires-cuda-gpu]") {
  auto mCpu{getExampleModel(Mod::SingleCompartmentDiffusion)};
  auto mGpu{getExampleModel(Mod::SingleCompartmentDiffusion)};
  configurePixelBackend(mCpu, simulate::PixelBackendType::CPU, singleStepDt);
  configurePixelBackend(mGpu, simulate::PixelBackendType::GPU, singleStepDt);

  const auto *field = mCpu.getSpecies().getField("slow");
  REQUIRE(field != nullptr);
  const auto inputConcentrations = makeRandomConcentrations(
      field->getConcentration().size(), 2, 67890, 0.0, 1.0);
  setRestartConcentrations(mCpu, inputConcentrations);
  setRestartConcentrations(mGpu, inputConcentrations);

  simulate::PixelSim cpuSim(mCpu, {"circle"}, {{"slow", "fast"}});
  simulate::CudaPixelSim gpuSim(mGpu, {"circle"}, {{"slow", "fast"}});
  REQUIRE(cpuSim.errorMessage().empty());
  if (!requireCudaReady(gpuSim)) {
    return;
  }

  const auto cpuSteps = cpuSim.run(singleStepDt, -1.0, {});
  const auto gpuSteps = gpuSim.run(singleStepDt, -1.0, {});
  REQUIRE(cpuSim.errorMessage().empty());
  REQUIRE(gpuSim.errorMessage().empty());
  REQUIRE(cpuSteps == 1);
  REQUIRE(gpuSteps == 1);

  requireNear(cpuSim.getDcdt(0), gpuSim.getDcdt(0));
  requireNear(cpuSim.getConcentrations(0), gpuSim.getConcentrations(0));
}

TEST_CASE("CudaPixelSim membrane reactions match CPU for one step",
          "[core/simulate/cudapixelsim][core/simulate][core][simulate]"
          "[expensive][requires-cuda-gpu]") {
  auto mCpu{getExampleModel(Mod::VerySimpleModel)};
  auto mGpu{getExampleModel(Mod::VerySimpleModel)};
  configurePixelBackend(mCpu, simulate::PixelBackendType::CPU, singleStepDt);
  configurePixelBackend(mGpu, simulate::PixelBackendType::GPU, singleStepDt);
  randomizeReactiveSpecies(mCpu, mGpu, 24680);

  simulate::Simulation cpuSim(mCpu);
  simulate::Simulation gpuSim(mGpu);
  REQUIRE(cpuSim.errorMessage().empty());
  if (isCudaRuntimeUnavailable(gpuSim.errorMessage())) {
    WARN("Skipping CUDA membrane comparison: " << gpuSim.errorMessage());
    return;
  }
  REQUIRE(gpuSim.errorMessage().empty());

  const auto cpuSteps = cpuSim.doTimesteps(singleStepDt, 1, -1.0);
  const auto gpuSteps = gpuSim.doTimesteps(singleStepDt, 1, -1.0);
  REQUIRE(cpuSim.errorMessage().empty());
  REQUIRE(gpuSim.errorMessage().empty());
  REQUIRE(cpuSteps == 1);
  REQUIRE(gpuSteps == 1);
  requireMatchingSimulationState(cpuSim, gpuSim, 1);
}

TEST_CASE("CudaPixelSim can run when constructed and executed on different "
          "threads",
          "[core/simulate/cudapixelsim][core/simulate][core][simulate]"
          "[requires-cuda-gpu]") {
  auto m{getExampleModel(Mod::ABtoC)};
  configurePixelBackend(m, simulate::PixelBackendType::GPU, singleStepDt);
  for (const auto &speciesId : {"A", "B", "C"}) {
    m.getSpecies().setDiffusionConstant(speciesId, 0.0);
  }

  simulate::Simulation sim(m);
  if (isCudaRuntimeUnavailable(sim.errorMessage())) {
    WARN("Skipping CUDA cross-thread run test: " << sim.errorMessage());
    return;
  }
  REQUIRE(sim.errorMessage().empty());

  auto future = std::async(std::launch::async, [&sim]() {
    return sim.doTimesteps(singleStepDt, 1, -1.0);
  });
  const auto steps = future.get();
  REQUIRE(steps == 1);
  REQUIRE(sim.errorMessage().empty());
  REQUIRE(sim.getTimePoints().size() == 2);
}

TEST_CASE("CudaPixelSim example-model sweep matches CPU for all example models",
          "[core/simulate/cudapixelsim][core/simulate][core][simulate]"
          "[expensive][requires-cuda-gpu]") {
  for (std::size_t iExample = 0; iExample < allExampleModels.size();
       ++iExample) {
    const auto exampleModel = allExampleModels[iExample];
    CAPTURE(toString(exampleModel));

    auto mCpu{getExampleModel(exampleModel)};
    auto mGpu{getExampleModel(exampleModel)};
    configurePixelBackend(mCpu, simulate::PixelBackendType::CPU, singleStepDt);
    configurePixelBackend(mGpu, simulate::PixelBackendType::GPU, singleStepDt);
    randomizeReactiveSpecies(mCpu, mGpu,
                             1000u + static_cast<std::uint32_t>(iExample));

    simulate::Simulation cpuSim(mCpu);
    simulate::Simulation gpuSim(mGpu);
    REQUIRE(cpuSim.errorMessage().empty());
    if (isCudaRuntimeUnavailable(gpuSim.errorMessage())) {
      WARN("Skipping CUDA example-model sweep: " << gpuSim.errorMessage());
      return;
    }
    REQUIRE(gpuSim.errorMessage().empty());

    const auto cpuSteps = cpuSim.doTimesteps(singleStepDt, 1, -1.0);
    const auto gpuSteps = gpuSim.doTimesteps(singleStepDt, 1, -1.0);
    REQUIRE(cpuSim.errorMessage().empty());
    REQUIRE(gpuSim.errorMessage().empty());
    REQUIRE(cpuSteps == 1);
    REQUIRE(gpuSteps == 1);
    requireMatchingSimulationState(cpuSim, gpuSim, 1);
  }
}

TEST_CASE(
    "CudaPixelSim membrane example model matches CPU after 1s",
    "[core/simulate/cudapixelsim][core/simulate][core][simulate][expensive]"
    "[requires-cuda-gpu]") {
  auto mCpu{getExampleModel(Mod::VerySimpleModel)};
  auto mGpu{getExampleModel(Mod::VerySimpleModel)};
  configurePixelBackend(mCpu, simulate::PixelBackendType::CPU,
                        longRunMaxTimestep);
  configurePixelBackend(mGpu, simulate::PixelBackendType::GPU,
                        longRunMaxTimestep);
  randomizeReactiveSpecies(mCpu, mGpu, 24681);

  simulate::Simulation cpuSim(mCpu);
  simulate::Simulation gpuSim(mGpu);
  REQUIRE(cpuSim.errorMessage().empty());
  if (isCudaRuntimeUnavailable(gpuSim.errorMessage())) {
    WARN("Skipping CUDA membrane 1s comparison: " << gpuSim.errorMessage());
    return;
  }
  REQUIRE(gpuSim.errorMessage().empty());

  const auto cpuSteps = cpuSim.doTimesteps(longRunTime, 1, -1.0);
  const auto gpuSteps = gpuSim.doTimesteps(longRunTime, 1, -1.0);
  REQUIRE(cpuSim.errorMessage().empty());
  REQUIRE(gpuSim.errorMessage().empty());
  REQUIRE(cpuSteps >= 1);
  REQUIRE(gpuSteps >= 1);

  const auto cpuTimes = cpuSim.getTimePoints();
  const auto gpuTimes = gpuSim.getTimePoints();
  REQUIRE(cpuTimes.size() >= 2);
  REQUIRE(gpuTimes.size() == cpuTimes.size());
  requireNear(cpuTimes, gpuTimes);
  REQUIRE(cpuTimes.back() == Catch::Approx(longRunTime)
                                 .epsilon(comparisonRelTol)
                                 .margin(comparisonAbsTol));
  requireMatchingSimulationState(cpuSim, gpuSim, cpuTimes.size() - 1);
}

TEST_CASE(
    "CudaPixelSim selected example models match CPU after 1s",
    "[core/simulate/cudapixelsim][core/simulate][core][simulate][expensive]"
    "[requires-cuda-gpu]") {
  std::size_t nModelsTested{0};
  for (std::size_t iExample = 0; iExample < longRunExampleModels.size();
       ++iExample) {
    const auto exampleModel = longRunExampleModels[iExample];
    auto mCpu{getExampleModel(exampleModel)};
    ++nModelsTested;
    CAPTURE(toString(exampleModel));

    auto mGpu{getExampleModel(exampleModel)};
    configurePixelBackend(mCpu, simulate::PixelBackendType::CPU,
                          longRunMaxTimestep);
    configurePixelBackend(mGpu, simulate::PixelBackendType::GPU,
                          longRunMaxTimestep);
    randomizeReactiveSpecies(mCpu, mGpu,
                             2000u + static_cast<std::uint32_t>(iExample));

    simulate::Simulation cpuSim(mCpu);
    simulate::Simulation gpuSim(mGpu);
    REQUIRE(cpuSim.errorMessage().empty());
    if (isCudaRuntimeUnavailable(gpuSim.errorMessage())) {
      WARN("Skipping CUDA 1s comparison sweep: " << gpuSim.errorMessage());
      return;
    }
    REQUIRE(gpuSim.errorMessage().empty());

    const auto cpuSteps = cpuSim.doTimesteps(longRunTime, 1, -1.0);
    const auto gpuSteps = gpuSim.doTimesteps(longRunTime, 1, -1.0);
    REQUIRE(cpuSim.errorMessage().empty());
    REQUIRE(gpuSim.errorMessage().empty());
    REQUIRE(cpuSteps >= 1);
    REQUIRE(gpuSteps >= 1);

    const auto cpuTimes = cpuSim.getTimePoints();
    const auto gpuTimes = gpuSim.getTimePoints();
    REQUIRE(cpuTimes.size() >= 2);
    REQUIRE(gpuTimes.size() == cpuTimes.size());
    requireNear(cpuTimes, gpuTimes);
    REQUIRE(cpuTimes.back() == Catch::Approx(longRunTime)
                                   .epsilon(comparisonRelTol)
                                   .margin(comparisonAbsTol));

    requireMatchingSimulationState(cpuSim, gpuSim, cpuTimes.size() - 1);
  }
  REQUIRE(nModelsTested > 0);
}

TEST_CASE(
    "CudaPixelSim selected example models match CPU after 1s with RK212",
    "[core/simulate/cudapixelsim][core/simulate][core][simulate][expensive]"
    "[requires-cuda-gpu]") {
  std::size_t nModelsTested{0};
  for (std::size_t iExample = 0; iExample < adaptiveLongRunExampleModels.size();
       ++iExample) {
    const auto exampleModel = adaptiveLongRunExampleModels[iExample];
    CAPTURE(toString(exampleModel));
    auto mCpu{getExampleModel(exampleModel)};
    auto mGpu{getExampleModel(exampleModel)};
    ++nModelsTested;

    configurePixelBackend(mCpu, simulate::PixelBackendType::CPU,
                          longRunMaxTimestep,
                          simulate::PixelIntegratorType::RK212);
    configurePixelBackend(mGpu, simulate::PixelBackendType::GPU,
                          longRunMaxTimestep,
                          simulate::PixelIntegratorType::RK212);
    randomizeReactiveSpecies(mCpu, mGpu,
                             3000u + static_cast<std::uint32_t>(iExample));

    simulate::Simulation cpuSim(mCpu);
    simulate::Simulation gpuSim(mGpu);
    REQUIRE(cpuSim.errorMessage().empty());
    if (isCudaRuntimeUnavailable(gpuSim.errorMessage())) {
      WARN("Skipping CUDA RK212 comparison sweep: " << gpuSim.errorMessage());
      return;
    }
    REQUIRE(gpuSim.errorMessage().empty());

    const auto cpuSteps = cpuSim.doTimesteps(longRunTime, 1, -1.0);
    const auto gpuSteps = gpuSim.doTimesteps(longRunTime, 1, -1.0);
    REQUIRE(cpuSim.errorMessage().empty());
    REQUIRE(gpuSim.errorMessage().empty());
    REQUIRE(cpuSteps >= 1);
    REQUIRE(gpuSteps >= 1);

    const auto cpuTimes = cpuSim.getTimePoints();
    const auto gpuTimes = gpuSim.getTimePoints();
    REQUIRE(cpuTimes.size() >= 2);
    REQUIRE(gpuTimes.size() == cpuTimes.size());
    requireNear(cpuTimes, gpuTimes);
    REQUIRE(cpuTimes.back() == Catch::Approx(longRunTime)
                                   .epsilon(comparisonRelTol)
                                   .margin(comparisonAbsTol));

    requireMatchingSimulationState(cpuSim, gpuSim, cpuTimes.size() - 1);
    requireMatchingLowerOrderState(cpuSim, gpuSim, cpuTimes.size() - 1);
  }
  REQUIRE(nModelsTested > 0);
}

TEST_CASE(
    "CudaPixelSim example-model sweep matches CPU with RK323 for one step",
    "[core/simulate/cudapixelsim][core/simulate][core][simulate][expensive]"
    "[requires-cuda-gpu]") {
  for (std::size_t iExample = 0; iExample < allExampleModels.size();
       ++iExample) {
    const auto exampleModel = allExampleModels[iExample];
    CAPTURE(toString(exampleModel));

    auto mCpu{getExampleModel(exampleModel)};
    auto mGpu{getExampleModel(exampleModel)};
    configurePixelBackend(mCpu, simulate::PixelBackendType::CPU, singleStepDt,
                          simulate::PixelIntegratorType::RK323);
    configurePixelBackend(mGpu, simulate::PixelBackendType::GPU, singleStepDt,
                          simulate::PixelIntegratorType::RK323);
    randomizeReactiveSpecies(mCpu, mGpu,
                             4000u + static_cast<std::uint32_t>(iExample));

    simulate::Simulation cpuSim(mCpu);
    simulate::Simulation gpuSim(mGpu);
    REQUIRE(cpuSim.errorMessage().empty());
    if (isCudaRuntimeUnavailable(gpuSim.errorMessage())) {
      WARN("Skipping CUDA RK323 sweep: " << gpuSim.errorMessage());
      return;
    }
    REQUIRE(gpuSim.errorMessage().empty());

    const auto cpuSteps = cpuSim.doTimesteps(singleStepDt, 1, -1.0);
    const auto gpuSteps = gpuSim.doTimesteps(singleStepDt, 1, -1.0);
    REQUIRE(cpuSim.errorMessage().empty());
    REQUIRE(gpuSim.errorMessage().empty());
    REQUIRE(cpuSteps >= 1);
    REQUIRE(gpuSteps >= 1);
    requireMatchingSimulationState(cpuSim, gpuSim, 1);
  }
}

TEST_CASE(
    "CudaPixelSim example-model sweep matches CPU with RK435 for one step",
    "[core/simulate/cudapixelsim][core/simulate][core][simulate][expensive]"
    "[requires-cuda-gpu]") {
  for (std::size_t iExample = 0; iExample < allExampleModels.size();
       ++iExample) {
    const auto exampleModel = allExampleModels[iExample];
    CAPTURE(toString(exampleModel));

    auto mCpu{getExampleModel(exampleModel)};
    auto mGpu{getExampleModel(exampleModel)};
    configurePixelBackend(mCpu, simulate::PixelBackendType::CPU, singleStepDt,
                          simulate::PixelIntegratorType::RK435);
    configurePixelBackend(mGpu, simulate::PixelBackendType::GPU, singleStepDt,
                          simulate::PixelIntegratorType::RK435);
    randomizeReactiveSpecies(mCpu, mGpu,
                             5000u + static_cast<std::uint32_t>(iExample));

    simulate::Simulation cpuSim(mCpu);
    simulate::Simulation gpuSim(mGpu);
    REQUIRE(cpuSim.errorMessage().empty());
    if (isCudaRuntimeUnavailable(gpuSim.errorMessage())) {
      WARN("Skipping CUDA RK435 sweep: " << gpuSim.errorMessage());
      return;
    }
    REQUIRE(gpuSim.errorMessage().empty());

    const auto cpuSteps = cpuSim.doTimesteps(singleStepDt, 1, -1.0);
    const auto gpuSteps = gpuSim.doTimesteps(singleStepDt, 1, -1.0);
    REQUIRE(cpuSim.errorMessage().empty());
    REQUIRE(gpuSim.errorMessage().empty());
    REQUIRE(cpuSteps >= 1);
    REQUIRE(gpuSteps >= 1);
    requireMatchingSimulationState(cpuSim, gpuSim, 1);
  }
}

TEST_CASE("CudaPixelSim float precision reaction matches double for one step",
          "[core/simulate/cudapixelsim][core/simulate][core][simulate]"
          "[expensive][requires-cuda-gpu]") {
  auto mDouble{getExampleModel(Mod::ABtoC)};
  auto mFloat{getExampleModel(Mod::ABtoC)};
  for (auto *m : {&mDouble, &mFloat}) {
    configurePixelBackend(*m, simulate::PixelBackendType::GPU, singleStepDt);
    for (const auto &speciesId : {"A", "B", "C"}) {
      m->getSpecies().setDiffusionConstant(speciesId, 0.0);
    }
  }
  mFloat.getSimulationSettings().options.pixel.gpuFloatPrecision =
      simulate::GpuFloatPrecision::Float;

  const auto *field = mDouble.getSpecies().getField("A");
  REQUIRE(field != nullptr);
  const auto inputConcentrations = makeRandomConcentrations(
      field->getConcentration().size(), 3, 77777, 0.05, 1.25);
  setRestartConcentrations(mDouble, inputConcentrations);
  setRestartConcentrations(mFloat, inputConcentrations);

  simulate::CudaPixelSim doubleSim(mDouble, {"comp"}, {{"A", "B", "C"}});
  simulate::CudaPixelSim floatSim(mFloat, {"comp"}, {{"A", "B", "C"}});
  if (!requireCudaReady(doubleSim) || !requireCudaReady(floatSim)) {
    return;
  }

  const auto doubleSteps = doubleSim.run(singleStepDt, -1.0, {});
  const auto floatSteps = floatSim.run(singleStepDt, -1.0, {});
  REQUIRE(doubleSim.errorMessage().empty());
  REQUIRE(floatSim.errorMessage().empty());
  REQUIRE(doubleSteps == 1);
  REQUIRE(floatSteps == 1);

  // float32 has ~7 decimal digits of precision, so we allow larger tolerance
  const auto &concDouble = doubleSim.getConcentrations(0);
  const auto &concFloat = floatSim.getConcentrations(0);
  REQUIRE(concDouble.size() == concFloat.size());
  for (std::size_t i = 0; i < concDouble.size(); ++i) {
    REQUIRE(concFloat[i] == Catch::Approx(concDouble[i]).epsilon(1e-5));
  }
}
