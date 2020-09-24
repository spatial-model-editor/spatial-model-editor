#include "cli_params.hpp"
#include "version.hpp"
#include <fmt/core.h>
#include <map>

namespace cli {

static void addParams(CLI::App &app, Params &params) {
  app.add_option("file", params.filename, "The spatial SBML model to simulate")
      ->required()
      ->check(CLI::ExistingFile);
  app.add_option("-t,--time", params.simulationTime,
                 "The simulation time (in model units of time)", true)
      ->check(CLI::PositiveNumber);
  app.add_option("-i,--image-interval", params.imageInterval,
                 "The interval between saving images (in model units of time)",
                 true)
      ->check(CLI::PositiveNumber);
  app.add_option("-s,--simulator", params.simType,
                 "The simulator to use: dune or pixel", true)
      ->transform(CLI::CheckedTransformer(
          std::map<std::string, simulate::SimulatorType>{
              {"dune", simulate::SimulatorType::DUNE},
              {"pixel", simulate::SimulatorType::Pixel}},
          CLI::ignore_case));
  app.add_option("-n,--nthreads", params.maxThreads,
                 "The maximum number of CPU threads to use (0 means unlimited)",
                 true)
      ->check(CLI::NonNegativeNumber);
}

static void addCallbacks(CLI::App &app) {
  app.add_flag_callback(
         "-v,--version",
         []() {
           fmt::print("{}", SPATIAL_MODEL_EDITOR_VERSION);
           throw CLI::Success();
         },
         "Display the version number and exit")
      ->configurable(false);
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
                               SPATIAL_MODEL_EDITOR_VERSION)});
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
  fmt::print("#   - Model: {}\n", params.filename);
  fmt::print("#   - Simulation Type: {}\n", toString(params.simType));
  fmt::print("#   - Simulation Length: {}\n", params.simulationTime);
  fmt::print("#   - Image Interval: {}\n", params.imageInterval);
  fmt::print("#   - Max CPU threads: {}\n", params.maxThreads);
}

} // namespace cli
