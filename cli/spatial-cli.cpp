#include <fmt/core.h>

#include <QFile>
#include <limits>

#include "logger.hpp"
#include "model.hpp"
#include "simulate.hpp"
#include "version.hpp"

static std::string toString(const simulate::SimulatorType &s) {
  if (s == simulate::SimulatorType::DUNE) {
    return "DUNE";
  } else if (s == simulate::SimulatorType::Pixel) {
    return "Pixel";
  }
  return {};
}

static void printHelpMessage() {
  fmt::print("\nUsage:\n\n");
  fmt::print("./spatial-cli sbml-model-file.xml "
             "[simulator=dune] [simulation_time=100] [image_interval=10] "
             "[max_cpu_threads=0]\n");
  fmt::print("\nPossible values for simulator:\n");
  fmt::print("  - DUNE\n");
  fmt::print("  - Pixel\n");
}

struct Params {
  std::string filename;
  simulate::SimulatorType simType = simulate::SimulatorType::DUNE;
  double simulationTime = 100.0;
  double imageInterval = 1.0;
  std::size_t maxThreads = 0;
};

static Params parseArgs(int argc, char *argv[]) {
  Params params;
  if (argc < 2) {
    printHelpMessage();
    exit(0);
  }
  if (std::string a = argv[1]; (a == "-h") || (a == "--help")) {
    printHelpMessage();
    exit(0);
  } else {
    params.filename = argv[1];
  }
  if (argc > 2) {
    if (std::string a = argv[2]; a[0] == 'p' || a[0] == 'P') {
      params.simType = simulate::SimulatorType::Pixel;
    } else if (a[0] == 'd' || a[0] == 'D') {
      params.simType = simulate::SimulatorType::DUNE;
    }
  }
  if (argc > 3) {
    params.simulationTime = std::stod(argv[3]);
  }
  if (argc > 4) {
    params.imageInterval = std::stod(argv[4]);
  }
  if (argc > 5) {
    params.maxThreads = static_cast<std::size_t>(std::stoi(argv[5]));
  }
  fmt::print("\n# Simulation parameters:\n");
  fmt::print("#   - Model: {}\n", params.filename);
  fmt::print("#   - Simulation Type: {}\n", toString(params.simType));
  fmt::print("#   - Simulation Length: {}\n", params.simulationTime);
  fmt::print("#   - Image Interval: {}\n", params.imageInterval);
  fmt::print("#   - Max CPU threads: {}\n", params.maxThreads);
  return params;
}

static void doSimulation(const Params &params) {
  // disable logging
  spdlog::set_level(spdlog::level::off);

  // import model
  model::Model s;
  QFile f(params.filename.c_str());
  if (f.open(QIODevice::ReadOnly)) {
    s.importSBMLString(f.readAll().toStdString());
  } else {
    fmt::print("\n\nError: failed to open model file '{}'\n\n",
               params.filename);
    exit(1);
  }
  if (!s.getIsValid() || !s.getGeometry().getIsValid()) {
    fmt::print("\n\nError: invalid model '{}'\n\n", params.filename);
    exit(1);
  }

  // setup simulator options
  simulate::Options options;
  if (params.simType == simulate::SimulatorType::DUNE) {
    options.dune.dt = params.imageInterval * 0.2;
  } else {
    options.pixel.enableMultiThreading = true;
    options.pixel.maxThreads = params.maxThreads;
    if (params.maxThreads == 1) {
      options.pixel.enableMultiThreading = false;
    }
  }
  simulate::Simulation sim(s, params.simType, options);

  fmt::print("# t = {} [img{}.png]\n", sim.getTimePoints().back(),
             sim.getTimePoints().size() - 1);
  while (sim.getTimePoints().back() < params.simulationTime) {
    sim.doTimestep(params.imageInterval);
    fmt::print("# t = {} [img{}.png]\n", sim.getTimePoints().back(),
               sim.getTimePoints().size() - 1);
  }
  for (std::size_t iTime = 0; iTime < sim.getTimePoints().size(); ++iTime) {
    sim.getConcImage(iTime, {}, true).save(QString("img%1.png").arg(iTime));
  }
}

int main(int argc, char *argv[]) {
  fmt::print("# Spatial Model Editor CLI v{}\n", SPATIAL_MODEL_EDITOR_VERSION);
  auto params = parseArgs(argc, argv);
  doSimulation(params);
  fmt::print("# Simulation complete.\n");
}
