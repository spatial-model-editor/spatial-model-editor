#include "resource_utils.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/simulate.hpp"
#include "sme/version.hpp"
#include <QElapsedTimer>
#include <fmt/core.h>
#include <limits>
#include <string>
#include <vector>

using namespace sme;

struct BackendConfig {
  std::string name;
  simulate::PixelBackendType pixelBackend;
  simulate::GpuFloatPrecision gpuPrecision;
};

static const std::vector<BackendConfig> allBackends{
    {"Pixel/CPU", simulate::PixelBackendType::CPU,
     simulate::GpuFloatPrecision::Double},
    {"Pixel/GPU(f64)", simulate::PixelBackendType::GPU,
     simulate::GpuFloatPrecision::Double},
    {"Pixel/GPU(f32)", simulate::PixelBackendType::GPU,
     simulate::GpuFloatPrecision::Float},
};

struct BenchmarkParams {
  int nTimesteps{1000};
  std::vector<const char *> models{
      "single-compartment-diffusion",
      "ABtoC",
      "very-simple-model",
      "brusselator-model",
      "circadian-clock",
      "gray-scott",
      "liver-simplified",
      "single-compartment-diffusion-3d",
      "very-simple-model-3d",
      "gray-scott-3d",
      "FitzhughNagumo3D",
      "SelKov3D",
      "CalciumWavePropagation3D",
  };
  double simulator_timestep{1e-4};
  std::vector<BackendConfig> backends{allBackends};
};

static void printHelpMessage() {
  BenchmarkParams params;
  fmt::print("\nUsage:\n");
  fmt::print("\n./benchmark [nTimesteps=1000] [model=all] "
             "[timestep=1e-4] [backend=all]\n");
  fmt::print("\nPossible values for model:\n");
  for (const auto &model : params.models) {
    fmt::print("  - {}\n", model);
  }
  fmt::print("  - all: all of the above\n");
  fmt::print("\nPossible values for backend:\n");
  for (const auto &backend : allBackends) {
    fmt::print("  - {}\n", backend.name);
  }
  fmt::print("  - all: all of the above\n");
}

static BenchmarkParams parseArgs(int argc, char *argv[]) {
  BenchmarkParams params;
  if (argc < 2) {
    return params;
  }
  if (std::string a = argv[1]; (a == "-h") || (a == "--help")) {
    printHelpMessage();
    exit(0);
  } else {
    params.nTimesteps = std::stoi(argv[1]);
  }
  if (argc > 2) {
    if (std::string arg = argv[2]; arg != "all") {
      if (auto iter = std::find_if(
              cbegin(params.models), cend(params.models),
              [&arg](const std::string &s) { return s.starts_with(arg); });
          iter != cend(params.models)) {
        params.models = {*iter};
      } else {
        fmt::print("\nERROR: model '{}' not found\n", arg);
        printHelpMessage();
        exit(1);
      }
    }
  }
  if (argc > 3) {
    params.simulator_timestep = std::stod(argv[3]);
  }
  if (argc > 4) {
    if (std::string arg = argv[4]; arg != "all") {
      auto it = std::find_if(
          allBackends.begin(), allBackends.end(),
          [&arg](const BackendConfig &b) { return b.name == arg; });
      if (it != allBackends.end()) {
        params.backends = {*it};
      } else {
        fmt::print("\nERROR: backend '{}' not found\n", arg);
        printHelpMessage();
        exit(1);
      }
    }
  }
  fmt::print("\n# Benchmark parameters:\n");
  fmt::print("# nTimesteps: {}\n", params.nTimesteps);
  fmt::print("# models:\n");
  for (const auto &model : params.models) {
    fmt::print("#   - {}\n", model);
  }
  fmt::print("# timestep: {}s\n", params.simulator_timestep);
  return params;
}

static std::string runBenchmark(model::Model &s, int nTimesteps, double dt) {
  simulate::Simulation sim(s);
  if (!sim.errorMessage().empty()) {
    return sim.errorMessage();
  }
  double totalTime = nTimesteps * dt;
  QElapsedTimer time;
  time.start();
  sim.doMultipleTimesteps({{1, totalTime}});
  if (!sim.errorMessage().empty()) {
    return sim.errorMessage();
  }
  double ms{static_cast<double>(time.elapsed()) /
            static_cast<double>(nTimesteps)};
  return fmt::format("{:.5f}", ms);
}

static void printBenchmarks(const BenchmarkParams &params) {
  Q_INIT_RESOURCE(resources);
  spdlog::set_level(spdlog::level::off);

  double dt = params.simulator_timestep;

  fmt::print("\n# {:36s}", "model (ms/timestep)");
  for (const auto &backend : params.backends) {
    fmt::print("\t{:>16s}", backend.name);
  }
  fmt::print("\n");

  for (const auto &model : params.models) {
    fmt::print("  {:36s}", model);
    fflush(stdout);

    for (const auto &backend : params.backends) {
      model::Model s;
      s.importSBMLString(benchmarking::readResourceTextFile(
          QString(":/models/%1.xml").arg(model)));

      auto &options = s.getSimulationSettings().options;
      options.pixel.backend = backend.pixelBackend;
      options.pixel.gpuFloatPrecision = backend.gpuPrecision;
      options.pixel.maxTimestep = dt;
      options.pixel.maxErr = {std::numeric_limits<double>::max(),
                              std::numeric_limits<double>::max()};
      s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;

      auto result = runBenchmark(s, params.nTimesteps, dt);
      // check if result is a number (success) or error message
      if (result[0] >= '0' && result[0] <= '9') {
        fmt::print("\t{:>16s}", result);
      } else {
        if (result.size() > 14) {
          result = result.substr(0, 11) + "...";
        }
        fmt::print("\t{:>16s}", result);
      }
      fflush(stdout);
    }
    fmt::print("\n");
  }
}

int main(int argc, char *argv[]) {
  fmt::print("# Spatial Model Editor v{}\n",
             common::SPATIAL_MODEL_EDITOR_VERSION);
  fmt::print("# Simulator benchmark code\n");
  auto benchmarkParams = parseArgs(argc, argv);
  printBenchmarks(benchmarkParams);
}
