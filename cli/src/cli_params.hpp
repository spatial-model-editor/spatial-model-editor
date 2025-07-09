#pragma once

#include "sme/optimize_options.hpp"
#include "sme/simulate.hpp"
#include <CLI/CLI.hpp>

namespace sme::cli {

struct SimParams {
  std::string simulationTimes;
  std::string imageIntervals;
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
  simulate::SimulatorType simType{simulate::SimulatorType::DUNE};
  std::string outputFile{};
  std::size_t maxThreads{0};
};

Params setupCLI(CLI::App &app);

std::string toString(const simulate::SimulatorType &s);

void printParams(const Params &params);

} // namespace sme::cli
