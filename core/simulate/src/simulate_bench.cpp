#include "bench.hpp"
#include "sme/simulate.hpp"
#include "sme/simulate_options.hpp"

using namespace sme;

template <typename T>
static void simulate_SimulationDUNE(benchmark::State &state) {
  T data;
  data.model.getSimulationSettings().simulatorType =
      simulate::SimulatorType::DUNE;
  std::unique_ptr<simulate::Simulation> simulation;
  for (auto _ : state) {
    simulation.reset();
    simulation = std::make_unique<simulate::Simulation>(data.model);
  }
}

template <typename T>
static void simulate_SimulationPIXEL(benchmark::State &state) {
  T data;
  data.model.getSimulationSettings().simulatorType =
      simulate::SimulatorType::Pixel;
  std::unique_ptr<simulate::Simulation> simulation;
  for (auto _ : state) {
    simulation = std::make_unique<simulate::Simulation>(data.model);
  }
}

template <typename T>
static void simulate_Simulation_getConcImage(benchmark::State &state) {
  T data;
  simulate::Simulation simulation(data.model);
  QImage img;
  for (auto _ : state) {
    img = simulation.getConcImage(0);
  }
}

SME_BENCHMARK(simulate_SimulationDUNE);
SME_BENCHMARK(simulate_SimulationPIXEL);
SME_BENCHMARK(simulate_Simulation_getConcImage);
