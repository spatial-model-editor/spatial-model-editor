#include "cli_command.hpp"
#include "cli_params.hpp"
#include <fmt/core.h>

int main(int argc, char *argv[]) {
  CLI::App app{"Spatial Model Editor CLI"};
  auto params{sme::cli::setupCLI(app)};
  try {
    app.parse(argc, argv);
    params.command = app.get_subcommands().at(0)->get_name();
  } catch (const CLI::ParseError &e) {
    return app.exit(e);
  }
  if (params.outputFile.empty()) {
    params.outputFile = params.inputFile;
  }
  sme::cli::printParams(params);
  bool success = sme::cli::runCommand(params);
  if (success) {
    fmt::print("# Done.\n");
  }
}
