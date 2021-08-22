#include "cli_params.hpp"
#include "version.hpp"
#include <fmt/core.h>
#include <map>

namespace sme::cli {

static void addParams(CLI::App &app, Params &params) {
  app.add_option("file", params.inputFile, "The spatial SBML model to simulate")
      ->required()
      ->check(CLI::ExistingFile);
  app.add_option("times", params.simulationTimes,
                 "The simulation time(s) (in model units of time)")
      ->required();
  app.add_option(
         "image-intervals", params.imageIntervals,
         "The interval(s) between saving images (in model units of time)")
      ->required();
  app.add_option("-s,--simulator", params.simType,
                 "The simulator to use: dune or pixel")
      ->transform(CLI::CheckedTransformer(
          std::map<std::string, simulate::SimulatorType>{
              {"dune", simulate::SimulatorType::DUNE},
              {"pixel", simulate::SimulatorType::Pixel}},
          CLI::ignore_case))
      ->capture_default_str();
  app.add_option("-o,--output-file", params.outputFile,
                 "The output file to write the results to. If not set, then "
                 "the input file is used.");
  app.add_option("-n,--nthreads", params.maxThreads,
                 "The maximum number of CPU threads to use (0 means unlimited)")
      ->check(CLI::NonNegativeNumber)
      ->capture_default_str();
}

static void addCallbacks(CLI::App &app) {
  app.set_version_flag("-v,--version", common::SPATIAL_MODEL_EDITOR_VERSION);
  app.add_flag_callback(
         "-d,--dump-config",
         [&app]() {
           fmt::print(app.config_to_str(true, true));
           throw CLI::Success();
         },
         "Dump the default config ini file and exit")
      ->configurable(false);
}

static void addConfig(CLI::App &app) {
  app.set_config("-c,--config", "",
                 "Read an ini file containing simulation options");
  app.config_formatter(std::make_shared<CLI::ConfigTOML>());
}

Params setupCLI(CLI::App &app) {
  Params params;
  app.description({fmt::format("Spatial Model Editor CLI v{}",
                               common::SPATIAL_MODEL_EDITOR_VERSION)});
  addParams(app, params);
  addCallbacks(app);
  addConfig(app);
  return params;
}

std::string toString(const simulate::SimulatorType &s) {
  if (s == simulate::SimulatorType::DUNE) {
    return "DUNE";
  } else if (s == simulate::SimulatorType::Pixel) {
    return "Pixel";
  }
  return {};
}

void printParams(const Params &params) {
  fmt::print("\n# Simulation parameters:\n");
  fmt::print("#   - Model: {}\n", params.inputFile);
  fmt::print("#   - Simulation Type: {}\n", toString(params.simType));
  fmt::print("#   - Simulation Length(s): {}\n", params.simulationTimes);
  fmt::print("#   - Image Interval(s): {}\n", params.imageIntervals);
  fmt::print("#   - Output file: {}\n", params.outputFile);
  fmt::print("#   - Max CPU threads: {}\n", params.maxThreads);
}

} // namespace sme::cli
