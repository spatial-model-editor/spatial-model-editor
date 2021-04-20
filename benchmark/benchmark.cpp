#include "logger.hpp"
#include "model.hpp"
#include "simulate.hpp"
#include "version.hpp"
#include <QElapsedTimer>
#include <QFile>
#include <fmt/core.h>
#include <limits>
#include <locale>
#include <memory>

using namespace sme;

static std::string toString(const simulate::SimulatorType &s) {
  if (s == simulate::SimulatorType::DUNE) {
    return "DUNE";
  } else if (s == simulate::SimulatorType::Pixel) {
    return "Pixel";
  }
  return {};
}

struct BenchmarkParams {
  int seconds_per_benchmark{10};
  std::vector<const char *> models{"single-compartment-diffusion",
                                   "ABtoC",
                                   "very-simple-model",
                                   "brusselator-model",
                                   "circadian-clock",
                                   "gray-scott",
                                   "liver-simplified"};
  std::vector<simulate::SimulatorType> simulators{
      simulate::SimulatorType::DUNE, simulate::SimulatorType::Pixel};
  double simulator_timestep{1e-4};
};

static void printHelpMessage() {
  BenchmarkParams params;
  fmt::print("\nUsage:\n");
  fmt::print("\n./benchmark [seconds_per_benchmark=10] [model=all] "
             "[simulator=all] [timestep=1e-3]\n");
  fmt::print("\nPossible values for model:\n");
  for (const auto &model : params.models) {
    fmt::print("  - {}\n", model);
  }
  fmt::print("  - all: all of the above\n");
  fmt::print("\nPossible values for simulator:\n");
  for (const auto &simulator : params.simulators) {
    fmt::print("  - {}\n", toString(simulator));
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
    params.seconds_per_benchmark = std::stoi(argv[1]);
  }
  if (argc > 2) {
    // models
    if (std::string arg = argv[2]; arg != "all") {
      if (auto iter = std::find_if(
              cbegin(params.models), cend(params.models),
              [&arg](const std::string &s) { return s[0] == arg[0]; });
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
    // simulators
    if (std::string a = argv[3]; a[0] == 'p' || a[0] == 'P') {
      params.simulators = {simulate::SimulatorType::Pixel};
    } else if (a[0] == 'd' || a[0] == 'D') {
      params.simulators = {simulate::SimulatorType::DUNE};
    }
  }
  if (argc > 4) {
    params.simulator_timestep = std::stod(argv[4]);
  }
  fmt::print("\n# Benchmark parameters:\n");
  fmt::print("# seconds_per_benchmark: {}s\n", params.seconds_per_benchmark);
  fmt::print("# models:\n");
  for (const auto &model : params.models) {
    fmt::print("#   - {}\n", model);
  }
  fmt::print("# simulators:\n");
  for (auto simulator : params.simulators) {
    fmt::print("#   - {}\n", toString(simulator));
  }
  fmt::print("# timestep: {}s\n", params.simulator_timestep);
  return params;
}

static void printSimulatorBenchmarks(const BenchmarkParams &params) {
  // resources contain example models
  Q_INIT_RESOURCE(resources);
  // disable logging
  spdlog::set_level(spdlog::level::off);
  // symengine assumes C locale
  std::locale::global(std::locale::classic());

  // fixed step size simulations
  double dt = params.simulator_timestep;
  simulate::Options options;
  options.pixel.maxTimestep = dt;
  options.pixel.maxErr = {std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max()};
  options.dune.dt = dt;
  options.dune.decrease = 0.9999999;
  options.dune.increase = 1.0000001;
  options.dune.maxDt = dt;
  options.dune.minDt = dt;

  for (auto simulator : params.simulators) {
    fmt::print("\n# {} simulator\n", toString(simulator));
    fmt::print("# ms/timestep\ttimesteps\tmodel\n");
    for (const auto &model : params.models) {
      // import model
      model::Model s;
      QFile f(QString(":/models/%1.xml").arg(model));
      f.open(QIODevice::ReadOnly);
      s.importSBMLString(f.readAll().toStdString());
      s.getSimulationSettings().options = options;
      s.getSimulationSettings().simulatorType = simulator;

      // setup simulator
      simulate::Simulation sim(s);
      // do a series of simulations
      // increase length of run by a factor of 2 each time
      // stop when a run takes more than half of runtime_seconds_per_model
      // and use this last run for the benchmark
      QElapsedTimer time;
      int iter {1};
      int ln2iter {0};
      long long elapsed_ms{0};
      while (elapsed_ms < params.seconds_per_benchmark * 500) {
        iter += iter;
        ++ln2iter;
        time.start();
        sim.doMultipleTimesteps({{iter, dt}});
        if (!sim.errorMessage().empty()) {
          fmt::print("Simulation error: {}\n", sim.errorMessage());
          exit(1);
        }
        elapsed_ms = time.elapsed();
      }
      double ms {static_cast<double>(elapsed_ms) / static_cast<double>(iter)};
      fmt::print("{:11.5f}\t2^{:<4}\t\t{}\n", ms, ln2iter, model);
    }
  }
}

int main(int argc, char *argv[]) {
  fmt::print("# Spatial Model Editor v{}\n", utils::SPATIAL_MODEL_EDITOR_VERSION);
  fmt::print("# Simulator benchmark code\n");
  auto benchmarkParams = parseArgs(argc, argv);
  printSimulatorBenchmarks(benchmarkParams);
}
