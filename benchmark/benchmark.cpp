#include <fmt/core.h>

#include <QElapsedTimer>
#include <QFile>
#include <limits>
#include <locale>

#include "logger.hpp"
#include "model.hpp"
#include "simulate.hpp"
#include "version.hpp"

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
  double simulator_timestep{1e-3};
  std::size_t simulator_integration_order = 1;
};

static void printHelpMessage() {
  BenchmarkParams params;
  fmt::print("\nUsage:\n");
  fmt::print(
      "\n./benchmark [seconds_per_benchmark=10] [model=all] "
      "[simulator=all] [timestep=1e-3] [order=1]\n");
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
  if (argc > 5) {
    params.simulator_integration_order =
        static_cast<std::size_t>(std::stoi(argv[5]));
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
  fmt::print("# order: {}\n", params.simulator_integration_order);
  return params;
}

static void printCpuBenchmark() {
  QElapsedTimer time;
  constexpr std::size_t iter = 67108864;
  double x = 399912349.2346234;
  time.start();
  for (std::size_t i = 0; i < iter; ++i) {
    x = sqrt(x);
  }
  auto elapsed = time.elapsed();
  double khz = static_cast<double>(iter) / static_cast<double>(elapsed);
  fmt::print("# CPU benchmark: sqrt frequency {:.3f} GHz\n", 1e-6 * khz);
}

static void printSimulatorBenchmarks(const BenchmarkParams &params) {
  // resources contain example models
  Q_INIT_RESOURCE(resources);
  // disable logging
  spdlog::set_level(spdlog::level::off);
  // symengine assumes C locale
  std::locale::global(std::locale::classic());

  double dt = params.simulator_timestep;

  for (auto simulator : params.simulators) {
    fmt::print("\n# {} simulator\n", toString(simulator));
    fmt::print("# ms/timestep\ttimesteps\tmodel\n");
    for (const auto &model : params.models) {
      // import model
      model::Model s;
      QFile f(QString(":/models/%1.xml").arg(model));
      f.open(QIODevice::ReadOnly);
      s.importSBMLString(f.readAll().toStdString());

      // setup simulator
      simulate::Simulation sim(s, simulator);
      auto options = sim.getIntegratorOptions();
      options.order = params.simulator_integration_order;
      options.maxAbsErr = std::numeric_limits<double>::max();
      options.maxRelErr = std::numeric_limits<double>::max();
      options.maxTimestep = dt;
      sim.setIntegratorOptions(options);

      // do a series of simulations
      // increase length of run by a factor of 2 each time
      // stop when a run takes more than half of runtime_seconds_per_model
      // and use this last run for the benchmark
      QElapsedTimer time;
      int iter = 1;
      int ln2iter = 0;
      long long elapsed_ms = 0;
      while (elapsed_ms < params.seconds_per_benchmark * 500) {
        iter += iter;
        ++ln2iter;
        time.start();
        sim.doTimestep(iter * dt);
        elapsed_ms = time.elapsed();
      }
      double ms = static_cast<double>(elapsed_ms) / static_cast<double>(iter);
      fmt::print("{:11.5f}\t2^{:<4}\t\t{}\n", ms, ln2iter, model);
    }
  }
}

int main(int argc, char *argv[]) {
  fmt::print("# Spatial Model Editor v{}\n", SPATIAL_MODEL_EDITOR_VERSION);
  fmt::print("# Simulator benchmark code\n");
  auto benchmarkParams = parseArgs(argc, argv);
  printCpuBenchmark();
  printSimulatorBenchmarks(benchmarkParams);
}
