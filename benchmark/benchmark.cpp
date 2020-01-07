#include <fmt/core.h>

#include <QElapsedTimer>
#include <QFile>
#include <locale>

#include "dune.hpp"
#include "logger.hpp"
#include "sbml.hpp"
#include "simulate.hpp"
#include "version.hpp"

static void printCpuBenchmark() {
  QElapsedTimer time;
  constexpr std::size_t iter = 67108864;
  double x = 399912349.2346234;
  time.start();
  for (std::size_t i = 0; i < iter; ++i) {
    x = sqrt(x);
  }
  auto elapsed = time.elapsed();
  double khz = static_cast<double>(iter) / static_cast<double>(elapsed);
  fmt::print("# CPU benchmark: sqrt frequency {:.3f} GHz\n", 1e-6 * khz);
}

int main(int argc, char *argv[]) {
  constexpr double dt = 1e-4;
  int runtime_seconds_per_model = 10;
  if (argc > 1) {
    runtime_seconds_per_model = std::stoi(argv[1]);
  }

  // resources contain example models
  Q_INIT_RESOURCE(resources);
  // disable logging
  spdlog::set_level(spdlog::level::off);
  // symengine assumes C locale
  std::locale::global(std::locale::classic());

  fmt::print("# Spatial Model Editor v{}\n", SPATIAL_MODEL_EDITOR_VERSION);
  fmt::print("# Simulator benchmarks\n");
  fmt::print("# Using ~{}s per benchmark\n", runtime_seconds_per_model);

  printCpuBenchmark();

  for (bool useDune : {false, true}) {
    if (useDune) {
      fmt::print("\n# DUNE simulator\n");
    } else {
      fmt::print("\n# Pixel simulator\n");
    }
    fmt::print("# ms/timestep\ttimesteps\tmodel\n");
    for (const auto &model :
         {"ABtoC", "very-simple-model", "brusselator-model", "circadian-clock",
          "gray-scott", "liver-simplified"}) {
      // import model
      sbml::SbmlDocWrapper s;
      QFile f(QString(":/models/%1.xml").arg(model));
      f.open(QIODevice::ReadOnly);
      s.importSBMLString(f.readAll().toStdString());

      // setup simulator
      std::unique_ptr<simulate::Simulate> simPixel;
      std::unique_ptr<dune::DuneSimulation> simDune;
      if (useDune) {
        simDune = std::make_unique<dune::DuneSimulation>(s, dt, QSize(1, 1));
      } else {
        simPixel = std::make_unique<simulate::Simulate>(&s);
        for (const auto &compartmentID : s.compartments) {
          simPixel->addCompartment(&s.mapCompIdToGeometry.at(compartmentID));
        }
        for (auto &membrane : s.membraneVec) {
          simPixel->addMembrane(&membrane);
        }
      }

      // do a series of simulations
      // increase length of run by a factor of 2 each time
      // stop when a run takes more than half of runtime_seconds_per_model
      // and use this last run for the benchmark
      QElapsedTimer time;
      int iter = 1;
      int ln2iter = 0;
      long long elapsed_ms = 0;
      while (elapsed_ms < runtime_seconds_per_model * 500) {
        iter += iter;
        ++ln2iter;
        time.start();
        if (useDune) {
          simDune->doTimestep(iter * dt);
        } else {
          for (int i = 0; i < iter; ++i) {
            simPixel->integrateForwardsEuler(dt);
          }
        }
        elapsed_ms = time.elapsed();
      }
      double ms = static_cast<double>(elapsed_ms) / static_cast<double>(iter);
      fmt::print("{:11.5f}\t2^{:<4}\t\t{}\n", ms, ln2iter, model);
    }
  }

  return 0;
}
