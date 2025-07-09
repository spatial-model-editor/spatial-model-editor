#include "cli_params.hpp"
#include "sme/version.hpp"
#include <fmt/core.h>
#include <map>

namespace sme::cli {

static void addParams(CLI::App &app, Params &params) {
  app.require_subcommand(1, 1);
  auto *sim_app = app.add_subcommand("simulate", "Run a spatial simulation");
  auto *fit_app = app.add_subcommand("fit", "Run parameter fitting");
  // common options
  for (auto *sub_app : {sim_app, fit_app}) {
    sub_app
        ->add_option("file", params.inputFile,
                     "The spatial SBML model to simulate")
        ->required()
        ->check(CLI::ExistingFile);
    sub_app
        ->add_option("-s,--simulator", params.simType,
                     "The simulator to use: dune or pixel")
        ->transform(CLI::CheckedTransformer(
            std::map<std::string, simulate::SimulatorType, std::less<>>{
                {"dune", simulate::SimulatorType::DUNE},
                {"pixel", simulate::SimulatorType::Pixel}},
            CLI::ignore_case))
        ->capture_default_str();
    sub_app
        ->add_option("-t,--nthreads", params.maxThreads,
                     "The maximum number of CPU threads to use when simulating "
                     "(0 means unlimited)")
        ->check(CLI::NonNegativeNumber)
        ->capture_default_str();
    sub_app->add_option(
        "-o,--output-file", params.outputFile,
        "The output file to write the results to. If not set, then "
        "the input file is used.");
  }
  // simulation options
  sim_app
      ->add_option(
          "times", params.sim.simulationTimes,
          "The simulation time(s) (in model units of time, separated by ';') ")
      ->required();
  sim_app
      ->add_option(
          "image-intervals", params.sim.imageIntervals,
          "The interval(s) between saving images (in model units of time)")
      ->required();
  // fitting options
  using enum sme::simulate::OptAlgorithmType;
  fit_app
      ->add_option("-a,--algorithm", params.fit.algorithm,
                   "The optimization algorithm to use")
      ->transform(CLI::CheckedTransformer(
          std::map<std::string, simulate::OptAlgorithmType, std::less<>>{
              {"PSO", PSO},
              {"GPSO", GPSO},
              {"DE", DE},
              {"iDE", iDE},
              {"jDE", jDE},
              {"pDE", pDE},
              {"ABC", ABC},
              {"gaco", gaco},
              {"COBYLA", COBYLA},
              {"BOBYQA", BOBYQA},
              {"NMS", NMS},
              {"sbplx", sbplx},
              {"AL", AL},
              {"PRAXIS", PRAXIS}},
          CLI::ignore_case))
      ->capture_default_str();
  fit_app
      ->add_option("-i,--n-iterations", params.fit.nIterations,
                   "The number of iterations to run the fitting algorithm")
      ->capture_default_str()
      ->check(CLI::PositiveNumber);
  fit_app
      ->add_option("-p,--population-per-thread", params.fit.populationPerThread,
                   "The population per optimization thread")
      ->capture_default_str()
      ->check(CLI::PositiveNumber);
  fit_app
      ->add_option("-j,--n-threads", params.fit.nThreads,
                   "The number of optimization threads")
      ->capture_default_str()
      ->check(CLI::PositiveNumber);
}

static void addCallbacks(CLI::App &app) {
  app.set_version_flag("-v,--version", common::SPATIAL_MODEL_EDITOR_VERSION);
  app.add_flag_callback(
         "-d,--dump-config",
         [&app]() {
           fmt::print("{}", app.config_to_str(true, true));
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
  if (params.command == "fit") {
    fmt::print("\n# Fitting parameters:\n");
  } else if (params.command == "simulate") {
    fmt::print("\n# Simulation parameters:\n");
  }
  fmt::print("#   - Input file: {}\n", params.inputFile);
  fmt::print("#   - Output file: {}\n", params.outputFile);
  fmt::print("#   - Simulation Type: {}\n", toString(params.simType));
  fmt::print("#   - Max CPU threads (simulation): {}\n", params.maxThreads);
  if (params.command == "fit") {
    fmt::print("#   - Optimization algorithm: {}\n",
               simulate::toString(params.fit.algorithm));
    fmt::print("#   - Number of optimization threads: {}\n",
               params.fit.nThreads);
    fmt::print("#   - Population per optimization thread: {}\n",
               params.fit.populationPerThread);
    fmt::print("#   - Number of iterations: {}\n", params.fit.nIterations);
  }
  if (params.command == "simulate") {
    fmt::print("#   - Simulation Length(s): {}\n", params.sim.simulationTimes);
    fmt::print("#   - Image Interval(s): {}\n", params.sim.imageIntervals);
  }
}

} // namespace sme::cli
