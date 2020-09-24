#include "cli_params.hpp"
#include "cli_simulate.hpp"
#include <fmt/core.h>

int main(int argc, char *argv[]) {
  CLI::App app;
  auto params = cli::setupCLI(app);
  CLI11_PARSE(app, argc, argv);
  cli::printParams(params);
  bool success = cli::doSimulation(params);
  if (success) {
    fmt::print("# Simulation complete.\n");
  }
}
