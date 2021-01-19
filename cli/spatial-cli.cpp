#include "cli_params.hpp"
#include "cli_simulate.hpp"
#include <fmt/core.h>

int main(int argc, char *argv[]) {
  CLI::App app;
  auto params = sme::cli::setupCLI(app);
  CLI11_PARSE(app, argc, argv);
  sme::cli::printParams(params);
  bool success = sme::cli::doSimulation(params);
  if (success) {
    fmt::print("# Simulation complete.\n");
  }
}
