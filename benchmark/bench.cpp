#include "bench.hpp"
#include "sme/logger.hpp"
#include <QtGlobal>
#include <benchmark/benchmark.h>

int main(int argc, char **argv) {
  // load resources for sample images, models etc
  Q_INIT_RESOURCE(resources);
  // disable logging output
  spdlog::set_level(spdlog::level::off);
  // contents of BENCHMARK_MAIN() copied from benchmark/benchmark.h:
  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();
}
