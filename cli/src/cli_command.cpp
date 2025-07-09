#include "cli_command.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/optimize.hpp"
#include "sme/simulate.hpp"
#include <QFile>
#include <fmt/core.h>
#include <fmt/ranges.h>

namespace sme::cli {

void printSimulationInfo(const sme::model::Model &model) {
  const auto &data{model.getSimulationData()};
  if (auto n{data.timePoints.size()}; n > 1) {
    fmt::print("\n# Continuing existing simulation with {} timepoints...\n", n);
  } else {
    fmt::print("\n# Starting new simulation...\n");
  }
}

void printSimulationTimes(
    const std::vector<std::pair<std::size_t, double>> &times) {
  fmt::print("\n# Simulation times:\n");
  for (auto [n, l] : times) {
    fmt::print("#   - {} steps of length {}\n", n, l);
  }
}

bool runCommand(const Params &params) {
  // disable logging
  spdlog::set_level(spdlog::level::off);

  // import model
  model::Model s;
  s.importFile(params.inputFile);
  if (!s.getIsValid() || !s.getGeometry().getIsValid()) {
    fmt::print("\n\nError: invalid model '{}'\n\n", params.inputFile);
    return false;
  }

  // setup simulator options
  s.getSimulationSettings().simulatorType = params.simType;
  auto &options{s.getSimulationSettings().options};
  options.pixel.enableMultiThreading = true;
  options.pixel.maxThreads = params.maxThreads;
  if (params.maxThreads == 1) {
    options.pixel.enableMultiThreading = false;
  }

  if (params.command == "simulate") {
    auto times{simulate::parseSimulationTimes(
        params.sim.simulationTimes.c_str(), params.sim.imageIntervals.c_str())};
    if (!times.has_value()) {
      fmt::print("\n\nError: failed to parse simulation times\n\n");
      return false;
    }
    printSimulationTimes(times.value());
    simulate::Simulation sim(s);
    if (const auto &e = sim.errorMessage(); !e.empty()) {
      fmt::print("\n\nError in simulation setup: {}\n\n", e);
      return false;
    }

    printSimulationInfo(s);

    sim.doMultipleTimesteps(times.value());
    if (const auto &e = sim.errorMessage(); !e.empty()) {
      fmt::print("\n\nError during simulation: {}\n\n", e);
      return false;
    }
  } else if (params.command == "fit") {
    s.getOptimizeOptions().optAlgorithm.optAlgorithmType = params.fit.algorithm;
    s.getOptimizeOptions().optAlgorithm.population =
        params.fit.populationPerThread;
    s.getOptimizeOptions().optAlgorithm.islands = params.fit.nThreads;
    simulate::Optimization optimization(s);
    fmt::print("# {:19}\t{:19}\n", "Fitness",
               fmt::join(optimization.getParamNames(), "\t"));
    optimization.evolve(params.fit.nIterations,
                        [](double fitness, const std::vector<double> &pars) {
                          fmt::print("{:.13e}\t{:.13e}\n", fitness,
                                     fmt::join(pars, "\t"));
                        });
    if (const auto &e = optimization.getErrorMessage(); !e.empty()) {
      fmt::print("\n\nError during optimization: {}\n\n", e);
      return false;
    }
    optimization.applyParametersToModel(&s);
  }
  s.exportSMEFile(params.outputFile);
  return true;
}

} // namespace sme::cli
