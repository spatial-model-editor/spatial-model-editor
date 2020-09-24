#include "cli_simulate.hpp"
#include "logger.hpp"
#include "model.hpp"
#include "simulate.hpp"
#include <QFile>
#include <fmt/core.h>

namespace cli {

bool doSimulation(const Params &params) {
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
    return false;
  }
  if (!s.getIsValid() || !s.getGeometry().getIsValid()) {
    fmt::print("\n\nError: invalid model '{}'\n\n", params.filename);
    return false;
  }

  // setup simulator options
  simulate::Options options;
  options.pixel.enableMultiThreading = true;
  options.pixel.maxThreads = params.maxThreads;
  if (params.maxThreads == 1) {
    options.pixel.enableMultiThreading = false;
  }
  simulate::Simulation sim(s, params.simType, options);
  if (const auto &e = sim.errorMessage(); !e.empty()) {
    fmt::print("\n\nError in simulation setup: {}\n\n", e);
    return false;
  }

  fmt::print("# t = {} [img{}.png]\n", sim.getTimePoints().back(),
             sim.getTimePoints().size() - 1);
  while (sim.getTimePoints().back() < params.simulationTime) {
    sim.doTimestep(params.imageInterval);
    if (const auto &e = sim.errorMessage(); !e.empty()) {
      fmt::print("\n\nError during simulation: {}\n\n", e);
      return false;
    }
    fmt::print("# t = {} [img{}.png]\n", sim.getTimePoints().back(),
               sim.getTimePoints().size() - 1);
  }
  for (std::size_t iTime = 0; iTime < sim.getTimePoints().size(); ++iTime) {
    sim.getConcImage(iTime, {}, true).save(QString("img%1.png").arg(iTime));
  }
  return true;
}

} // namespace cli
