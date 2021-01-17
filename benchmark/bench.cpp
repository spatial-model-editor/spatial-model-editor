#include "logger.hpp"
#include "bench.hpp"
#include <QtGlobal>
#include <benchmark/benchmark.h>
#include "dune_headers.hpp"

int main(int argc, char **argv) {
  // load resources for sample images, models etc
  Q_INIT_RESOURCE(resources);
  Q_INIT_RESOURCE(test_resources);
  // disable logging output
  spdlog::set_level(spdlog::level::off);
  // init and mute DUNE logging
  Dune::Logging::Logging::init(
      Dune::FakeMPIHelper::getCollectiveCommunication());
  Dune::Logging::Logging::mute();
  // BENCHMARK_MAIN() from benchmark/benchmark.h without int main():
  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();
}
