#include <fmt/core.h>

#include <QElapsedTimer>
#include <QFile>
#include <locale>

#include "logger.hpp"
#include "sbml.hpp"
#include "simulate.hpp"
#include "version.hpp"

struct PixelParams {
  std::size_t integration_order = 1;
  std::vector<const char *> models{"single-compartment-diffusion",
                                   "ABtoC",
                                   "very-simple-model",
                                   "brusselator-model",
                                   "circadian-clock",
                                   "gray-scott",
                                   "liver-simplified"};
  double integration_time = 10.0;
  std::size_t compartment_index = 0;
  std::size_t species_index = 0;
  std::size_t pixel_index = 0;
};

static void printHelpMessage() {
  PixelParams params;
  fmt::print("\nUsage:\n");
  fmt::print(
      "\n./pixel [integration_order=1] [model=brusselator-model] "
      "[integration_time=10.0] [compartment_index=0] [species_index=0] "
      "[pixel_index=0]\n");
  fmt::print("\nPossible values for model:\n");
  for (const auto &model : params.models) {
    fmt::print("  - {}\n", model);
  }
}

static PixelParams parseArgs(int argc, char *argv[]) {
  PixelParams params;
  if (argc < 2) {
    params.models = {"brusselator-model"};
  } else if (std::string a = argv[1]; (a == "-h") || (a == "--help")) {
    printHelpMessage();
    exit(0);
  } else {
    params.integration_order = static_cast<std::size_t>(std::stoi(argv[1]));
  }
  if (argc > 2) {
    // models
    std::string arg = argv[2];
    if (auto iter = std::find_if(
            cbegin(params.models), cend(params.models),
            [&arg](const std::string &s) { return s[0] == arg[0]; });
        iter != cend(params.models)) {
      params.models = {*iter};
    } else {
      params.models = {"brusselator-model"};
    }
  }
  if (argc > 3) {
    params.integration_time = std::stod(argv[3]);
  }
  if (argc > 4) {
    params.compartment_index = static_cast<std::size_t>(std::stod(argv[4]));
  }
  if (argc > 5) {
    params.species_index = static_cast<std::size_t>(std::stod(argv[5]));
  }
  if (argc > 6) {
    params.pixel_index = static_cast<std::size_t>(std::stod(argv[6]));
  }
  fmt::print("\n# Pixel test parameters:\n");
  fmt::print("# integration_order: {}\n", params.integration_order);
  fmt::print("# model: {}\n", params.models[0]);
  fmt::print("# integration_time: {}\n", params.integration_time);
  fmt::print("# compartment_index: {}\n", params.compartment_index);
  fmt::print("# species_index: {}\n", params.species_index);
  fmt::print("# pixel_index: {}\n\n", params.pixel_index);
  return params;
}

static void printFixedTimestepPixel(const PixelParams &params) {
  // resources contain example models
  Q_INIT_RESOURCE(resources);
  // disable logging
  spdlog::set_level(spdlog::level::off);
  // symengine assumes C locale
  std::locale::global(std::locale::classic());

  fmt::print("# timestep\tvalue\t\t\tlower-order\t\truntime\tsteps\tmodel\n");
  for (double dt : {1.0,     0.5,      0.25,      0.125,   0.1,     0.05,
                    0.025,   0.02,     0.0125,    0.01,    0.005,   0.0025,
                    0.001,   0.0005,   0.00025,   0.0001,  0.00005, 0.000025,
                    0.00001, 0.000005, 0.0000025, 0.000001}) {
    // import model
    sbml::SbmlDocWrapper s;
    QFile f(QString(":/models/%1.xml").arg(params.models[0]));
    f.open(QIODevice::ReadOnly);
    s.importSBMLString(f.readAll().toStdString());

    // setup simulator
    simulate::Simulation sim(s, simulate::SimulatorType::Pixel);
    sim.setIntegrationOrder(params.integration_order);

    QElapsedTimer time;
    long long elapsed_ms = 0;
    time.start();
    std::size_t steps = sim.doTimestep(params.integration_time,
                                       std::numeric_limits<double>::max(), dt);
    elapsed_ms = time.elapsed();
    fmt::print(
        "{:11.8f}\t{:20.16e}\t{:20.16e}\t{}\t{}\t{}\n", dt,
        sim.getConc(sim.getTimePoints().size() - 1, params.compartment_index,
                    params.species_index)[params.pixel_index],
        sim.getLowerOrderConc(params.compartment_index, params.species_index,
                              params.pixel_index),
        elapsed_ms, steps, params.models[0]);
  }
}

int main(int argc, char *argv[]) {
  fmt::print("# Spatial Model Editor v{}\n", SPATIAL_MODEL_EDITOR_VERSION);
  fmt::print("# Pixel integrator test code\n");
  auto params = parseArgs(argc, argv);
  printFixedTimestepPixel(params);
}
