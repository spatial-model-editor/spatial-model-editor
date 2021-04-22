#pragma once

#include "simulate.hpp"
#include <CLI/CLI.hpp>

namespace sme::cli {

struct Params {
  std::string inputFile;
  std::string simulationTimes;
  std::string imageIntervals;
  simulate::SimulatorType simType{simulate::SimulatorType::DUNE};
  std::string outputFile{};
  std::size_t maxThreads{0};
};

Params setupCLI(CLI::App &app);

std::string toString(const simulate::SimulatorType &s);

void printParams(const Params &params);

} // namespace sme::cli
