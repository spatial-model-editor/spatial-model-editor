#pragma once

#include "sme/optimize_options.hpp"
#include "sme/simulate.hpp"
#include <CLI/CLI.hpp>
#include <optional>
#include <string>

namespace sme::cli {

struct SimParams {
  std::string simulationTimes;
  std::string imageIntervals;
  double timeoutSeconds{-1.0};
  bool throwOnTimeout{true};
  bool continueExistingSimulation{true};
  std::optional<std::string> duneIntegrator{};
  std::optional<double> duneInitialTimestep{};
  std::optional<double> duneMinTimestep{};
  std::optional<double> duneMaxTimestep{};
  std::optional<double> duneIncreaseFactor{};
  std::optional<double> duneDecreaseFactor{};
  std::optional<bool> duneOutputVtkFiles{};
  std::optional<double> duneNewtonRelativeError{};
  std::optional<double> duneNewtonAbsoluteError{};
  std::optional<std::string> duneLinearSolver{};
  std::optional<std::size_t> duneMaxThreads{};
  std::optional<simulate::PixelIntegratorType> pixelIntegrator{};
  std::optional<double> pixelMaxRelativeError{};
  std::optional<double> pixelMaxAbsoluteError{};
  std::optional<double> pixelMaxTimestep{};
  std::optional<bool> pixelEnableMultithreading{};
  std::optional<std::size_t> pixelMaxThreads{};
  std::optional<bool> pixelDoCSE{};
  std::optional<unsigned> pixelOptLevel{};
};

struct FitParams {
  std::size_t nIterations{20};
  std::size_t populationPerThread{20};
  std::size_t nThreads{1};
  simulate::OptAlgorithmType algorithm{simulate::OptAlgorithmType::PSO};
};

struct Params {
  std::string command;
  SimParams sim{};
  FitParams fit{};
  std::string inputFile;
  std::optional<simulate::SimulatorType> simType{};
  std::string outputFile{};
  std::optional<std::size_t> maxThreads{};
};

Params setupCLI(CLI::App &app);

std::string toString(const simulate::SimulatorType &s);

void printParams(const Params &params);

} // namespace sme::cli
