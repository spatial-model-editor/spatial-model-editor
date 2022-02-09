#include "bench.hpp"
#include "sme/model.hpp"

using namespace sme;

template <typename T> static void model_Model(benchmark::State &state) {
  T data;
  model::Model model;
  for (auto _ : state) {
    model.importSBMLString(data.xml);
    model.clear();
  }
}

template <typename T> static void model_Model_getXml(benchmark::State &state) {
  T data;
  model::Model model;
  model.importSBMLString(data.xml);
  QString xml;
  for (auto _ : state) {
    xml = model.getXml();
  }
}

SME_BENCHMARK(model_Model);
SME_BENCHMARK(model_Model_getXml);
