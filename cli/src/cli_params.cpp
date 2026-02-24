#include "cli_params.hpp"
#include "sme/version.hpp"
#include <fmt/core.h>
#include <map>

namespace sme::cli {

static auto makeSimulatorTypeMap() {
  return std::map<std::string, simulate::SimulatorType, std::less<>>{
      {"dune", simulate::SimulatorType::DUNE},
      {"pixel", simulate::SimulatorType::Pixel}};
}

static auto makeDuneIntegratorMap() {
  return std::map<std::string, std::string, std::less<>>{
      {"expliciteuler", "ExplicitEuler"},
      {"impliciteuler", "ImplicitEuler"},
      {"heun", "Heun"},
      {"fractionalsteptheta", "FractionalStepTheta"},
      {"alexander2", "Alexander2"},
      {"shu3", "Shu3"},
      {"alexander3", "Alexander3"},
      {"rungekutta4", "RungeKutta4"}};
}

static auto makeDuneLinearSolverMap() {
  return std::map<std::string, std::string, std::less<>>{
      {"bicgstab", "BiCGSTAB"},
      {"cg", "CG"},
      {"restartedgmres", "RestartedGMRes"},
      {"umfpack", "UMFPack"},
      {"superlu", "SuperLU"}};
}

static auto makePixelIntegratorMap() {
  return std::map<std::string, simulate::PixelIntegratorType, std::less<>>{
      {"rk101", simulate::PixelIntegratorType::RK101},
      {"rk212", simulate::PixelIntegratorType::RK212},
      {"rk323", simulate::PixelIntegratorType::RK323},
      {"rk435", simulate::PixelIntegratorType::RK435}};
}

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
        ->transform(
            CLI::CheckedTransformer(makeSimulatorTypeMap(), CLI::ignore_case));
    sub_app
        ->add_option("-t,--max-threads,--nthreads", params.maxThreads,
                     "The maximum number of CPU threads to use when simulating "
                     "(0 means unlimited). This sets both DUNE and Pixel "
                     "thread limits.")
        ->check(CLI::NonNegativeNumber);
    sub_app
        ->add_option("--dune-integrator", params.sim.duneIntegrator,
                     "DUNE integrator: expliciteuler, impliciteuler, heun, "
                     "fractionalsteptheta, alexander2, shu3, alexander3, or "
                     "rungekutta4")
        ->transform(
            CLI::CheckedTransformer(makeDuneIntegratorMap(), CLI::ignore_case));
    sub_app->add_option("--dune-initial-timestep",
                        params.sim.duneInitialTimestep,
                        "DUNE initial timestep");
    sub_app->add_option("--dune-min-timestep", params.sim.duneMinTimestep,
                        "DUNE minimum timestep");
    sub_app->add_option("--dune-max-timestep", params.sim.duneMaxTimestep,
                        "DUNE maximum timestep");
    sub_app->add_option("--dune-increase-factor", params.sim.duneIncreaseFactor,
                        "DUNE timestep increase factor");
    sub_app->add_option("--dune-decrease-factor", params.sim.duneDecreaseFactor,
                        "DUNE timestep decrease factor");
    sub_app->add_option("--dune-output-vtk-files",
                        params.sim.duneOutputVtkFiles,
                        "DUNE output VTK files (true/false)");
    sub_app->add_option("--dune-newton-relative-error",
                        params.sim.duneNewtonRelativeError,
                        "DUNE Newton relative error");
    sub_app->add_option("--dune-newton-absolute-error",
                        params.sim.duneNewtonAbsoluteError,
                        "DUNE Newton absolute error");
    sub_app
        ->add_option("--dune-linear-solver", params.sim.duneLinearSolver,
                     "DUNE linear solver: bicgstab, cg, restartedgmres, "
                     "umfpack, or superlu")
        ->transform(CLI::CheckedTransformer(makeDuneLinearSolverMap(),
                                            CLI::ignore_case));
    sub_app->add_option("--dune-max-threads", params.sim.duneMaxThreads,
                        "DUNE max CPU threads (0 means unlimited)");
    sub_app
        ->add_option("--pixel-integrator", params.sim.pixelIntegrator,
                     "Pixel integrator: rk101, rk212, rk323, or rk435")
        ->transform(CLI::CheckedTransformer(makePixelIntegratorMap(),
                                            CLI::ignore_case));
    sub_app->add_option("--pixel-max-relative-error",
                        params.sim.pixelMaxRelativeError,
                        "Pixel max relative local error");
    sub_app->add_option("--pixel-max-absolute-error",
                        params.sim.pixelMaxAbsoluteError,
                        "Pixel max absolute local error");
    sub_app->add_option("--pixel-max-timestep", params.sim.pixelMaxTimestep,
                        "Pixel max timestep");
    sub_app->add_option("--pixel-enable-multithreading",
                        params.sim.pixelEnableMultithreading,
                        "Enable Pixel multithreading (true/false)");
    sub_app->add_option("--pixel-max-threads", params.sim.pixelMaxThreads,
                        "Pixel max CPU threads (0 means unlimited)");
    sub_app->add_option("--pixel-do-cse", params.sim.pixelDoCSE,
                        "Enable Pixel common subexpression elimination "
                        "(true/false)");
    sub_app
        ->add_option("--pixel-opt-level", params.sim.pixelOptLevel,
                     "Pixel compiler optimization level (0-3)")
        ->check(CLI::Range(0, 3));
    sub_app->add_option(
        "-o,--output-file", params.outputFile,
        "The output file to write the results to. If not set, then "
        "the input file is used.");
  }
  // simulation options
  sim_app->add_option(
      "times", params.sim.simulationTimes,
      "The simulation time(s) (in model units of time, separated by ';'). "
      "Optional if image-intervals is also omitted, in which case model "
      "simulation settings are used.");
  sim_app->add_option(
      "image-intervals", params.sim.imageIntervals,
      "The interval(s) between saving images (in model units of time). "
      "Optional if times is also omitted, in which case model simulation "
      "settings are used.");
  sim_app
      ->add_option("--timeout-seconds", params.sim.timeoutSeconds,
                   "Simulation timeout in seconds (-1 means no timeout)")
      ->capture_default_str();
  sim_app
      ->add_option("--throw-on-timeout", params.sim.throwOnTimeout,
                   "Whether to treat timeout as an error (true/false)")
      ->capture_default_str();
  sim_app
      ->add_option("--continue-existing-simulation",
                   params.sim.continueExistingSimulation,
                   "Whether to continue existing simulation results from the "
                   "input model (true/false)")
      ->capture_default_str();
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
  auto boolToString = [](bool b) { return b ? "true" : "false"; };
  if (params.command == "fit") {
    fmt::print("\n# Fitting parameters:\n");
  } else if (params.command == "simulate") {
    fmt::print("\n# Simulation parameters:\n");
  }
  fmt::print("#   - Input file: {}\n", params.inputFile);
  fmt::print("#   - Output file: {}\n", params.outputFile);
  fmt::print("#   - Simulation Type: {}\n",
             params.simType.has_value() ? toString(params.simType.value())
                                        : "(from model)");
  fmt::print("#   - Max CPU threads (simulation): {}\n",
             params.maxThreads.has_value()
                 ? fmt::format("{}", params.maxThreads.value())
                 : "(from model)");
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
    fmt::print("#   - Simulation Length(s): {}\n",
               params.sim.simulationTimes.empty() ? "(from model)"
                                                  : params.sim.simulationTimes);
    fmt::print("#   - Image Interval(s): {}\n",
               params.sim.imageIntervals.empty() ? "(from model)"
                                                 : params.sim.imageIntervals);
    fmt::print("#   - Timeout (seconds): {}\n", params.sim.timeoutSeconds);
    fmt::print("#   - Throw on timeout: {}\n",
               boolToString(params.sim.throwOnTimeout));
    fmt::print("#   - Continue existing simulation: {}\n",
               boolToString(params.sim.continueExistingSimulation));
  }
}

} // namespace sme::cli
