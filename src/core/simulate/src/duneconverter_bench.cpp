#include "bench.hpp"
#include "duneconverter.hpp"

using namespace sme;

template <typename T>
static void simulate_DuneConverter(benchmark::State &state) {
  T data;
  std::unique_ptr<simulate::DuneConverter> duneConverter;
  for (auto _ : state) {
    duneConverter = std::make_unique<simulate::DuneConverter>(data.model);
  }
}

SME_BENCHMARK(simulate_DuneConverter);
