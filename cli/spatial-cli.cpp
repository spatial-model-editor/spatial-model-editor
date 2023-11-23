#include "cli_params.hpp"
#include "cli_simulate.hpp"
#include <fmt/core.h>

int main(int argc, char *argv[]) {
  CLI::App app{"Spatial Model Editor CLI"};
  auto params{sme::cli::setupCLI(app)};
  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    return app.exit(e);
  }
  if (params.outputFile.empty()) {
    params.outputFile = params.inputFile;
  }
  sme::cli::printParams(params);
  bool success{sme::cli::doSimulation(params)};
  if (success) {
    fmt::print("# Simulation complete.\n");
  }
}
