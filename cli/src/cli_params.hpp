#pragma once

#include "simulate.hpp"
#include <CLI/CLI.hpp>

namespace sme {

namespace cli {

struct Params {
  std::string filename;
  simulate::SimulatorType simType = simulate::SimulatorType::DUNE;
  double simulationTime{100.0};
  double imageInterval{1.0};
  std::size_t maxThreads{0};
};

Params setupCLI(CLI::App &app);

std::string toString(const simulate::SimulatorType &s);

void printParams(const Params &params);

} // namespace cli

} // namespace sme
