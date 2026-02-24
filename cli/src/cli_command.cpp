#include "cli_command.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/optimize.hpp"
#include "sme/simulate.hpp"
#include <QFile>
#include <fmt/core.h>
#include <fmt/ranges.h>

namespace sme::cli {

static bool simulationStoppedOrTimedOut(const std::string &msg) {
  return msg == "Simulation timeout" || msg == "Simulation stopped early";
}

static void applySimulationOptionOverrides(model::Model &model,
                                           const Params &params) {
  auto &settings{model.getSimulationSettings()};
  if (params.simType.has_value()) {
    settings.simulatorType = params.simType.value();
  }
  auto &opt{settings.options};
  if (params.maxThreads.has_value()) {
    opt.dune.maxThreads = params.maxThreads.value();
    opt.pixel.maxThreads = params.maxThreads.value();
    if (!params.sim.pixelEnableMultithreading.has_value()) {
      opt.pixel.enableMultiThreading = (params.maxThreads.value() != 1);
    }
  }
  if (params.sim.duneIntegrator.has_value()) {
    opt.dune.integrator = params.sim.duneIntegrator.value();
  }
  if (params.sim.duneInitialTimestep.has_value()) {
    opt.dune.dt = params.sim.duneInitialTimestep.value();
  }
  if (params.sim.duneMinTimestep.has_value()) {
    opt.dune.minDt = params.sim.duneMinTimestep.value();
  }
  if (params.sim.duneMaxTimestep.has_value()) {
    opt.dune.maxDt = params.sim.duneMaxTimestep.value();
  }
  if (params.sim.duneIncreaseFactor.has_value()) {
    opt.dune.increase = params.sim.duneIncreaseFactor.value();
  }
  if (params.sim.duneDecreaseFactor.has_value()) {
    opt.dune.decrease = params.sim.duneDecreaseFactor.value();
  }
  if (params.sim.duneOutputVtkFiles.has_value()) {
    opt.dune.writeVTKfiles = params.sim.duneOutputVtkFiles.value();
  }
  if (params.sim.duneNewtonRelativeError.has_value()) {
    opt.dune.newtonRelErr = params.sim.duneNewtonRelativeError.value();
  }
  if (params.sim.duneNewtonAbsoluteError.has_value()) {
    opt.dune.newtonAbsErr = params.sim.duneNewtonAbsoluteError.value();
  }
  if (params.sim.duneLinearSolver.has_value()) {
    opt.dune.linearSolver = params.sim.duneLinearSolver.value();
  }
  if (params.sim.duneMaxThreads.has_value()) {
    opt.dune.maxThreads = params.sim.duneMaxThreads.value();
  }
  if (params.sim.pixelIntegrator.has_value()) {
    opt.pixel.integrator = params.sim.pixelIntegrator.value();
  }
  if (params.sim.pixelMaxRelativeError.has_value()) {
    opt.pixel.maxErr.rel = params.sim.pixelMaxRelativeError.value();
  }
  if (params.sim.pixelMaxAbsoluteError.has_value()) {
    opt.pixel.maxErr.abs = params.sim.pixelMaxAbsoluteError.value();
  }
  if (params.sim.pixelMaxTimestep.has_value()) {
    opt.pixel.maxTimestep = params.sim.pixelMaxTimestep.value();
  }
  if (params.sim.pixelEnableMultithreading.has_value()) {
    opt.pixel.enableMultiThreading =
        params.sim.pixelEnableMultithreading.value();
  }
  if (params.sim.pixelMaxThreads.has_value()) {
    opt.pixel.maxThreads = params.sim.pixelMaxThreads.value();
    if (!params.sim.pixelEnableMultithreading.has_value()) {
      opt.pixel.enableMultiThreading =
          (params.sim.pixelMaxThreads.value() != 1);
    }
  }
  if (params.sim.pixelDoCSE.has_value()) {
    opt.pixel.doCSE = params.sim.pixelDoCSE.value();
  }
  if (params.sim.pixelOptLevel.has_value()) {
    opt.pixel.optLevel = params.sim.pixelOptLevel.value();
  }
}

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

  applySimulationOptionOverrides(s, params);

  if (params.command == "simulate") {
    const auto currentTimes{s.getSimulationSettings().times};
    if (!params.sim.continueExistingSimulation) {
      s.getSimulationData().clear();
      s.getSimulationSettings().times.clear();
    }
    const bool hasSimulationTimes = !params.sim.simulationTimes.empty();
    const bool hasImageIntervals = !params.sim.imageIntervals.empty();
    if (hasSimulationTimes != hasImageIntervals) {
      fmt::print("\n\nError: simulation times and image intervals must both "
                 "be set or both be omitted\n\n");
      return false;
    }
    std::vector<std::pair<std::size_t, double>> times;
    if (hasSimulationTimes) {
      auto parsedTimes{
          simulate::parseSimulationTimes(params.sim.simulationTimes.c_str(),
                                         params.sim.imageIntervals.c_str())};
      if (!parsedTimes.has_value()) {
        fmt::print("\n\nError: failed to parse simulation times\n\n");
        return false;
      }
      times = parsedTimes.value();
    } else {
      times = currentTimes;
      if (times.empty()) {
        fmt::print("\n\nError: no simulation times specified. Provide times "
                   "and image intervals, or use a model with existing "
                   "simulation settings.\n\n");
        return false;
      }
    }
    printSimulationTimes(times);
    simulate::Simulation sim(s);
    if (const auto &e = sim.errorMessage(); !e.empty()) {
      fmt::print("\n\nError in simulation setup: {}\n\n", e);
      return false;
    }

    printSimulationInfo(s);

    const auto timeoutMilliseconds = params.sim.timeoutSeconds < 0.0
                                         ? -1.0
                                         : 1000.0 * params.sim.timeoutSeconds;
    sim.doMultipleTimesteps(times, timeoutMilliseconds);
    if (const auto &e = sim.errorMessage();
        !e.empty() &&
        (params.sim.throwOnTimeout || !simulationStoppedOrTimedOut(e))) {
      fmt::print("\n\nError during simulation: {}\n\n", e);
      return false;
    } else if (const auto &e = sim.errorMessage();
               !e.empty() && !params.sim.throwOnTimeout) {
      fmt::print("\n# Simulation exited early: {}\n", e);
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
