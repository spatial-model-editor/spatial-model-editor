#include "logger.hpp"
#include <QtGlobal>
#include <benchmark/benchmark.h>

int main(int argc, char **argv) {
  // load resources for sample images, models etc
  Q_INIT_RESOURCE(resources);
  Q_INIT_RESOURCE(test_resources);
  // disable logging output
  spdlog::set_level(spdlog::level::off);
  // BENCHMARK_MAIN macro:
  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();
}
