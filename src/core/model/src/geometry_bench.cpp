#include "bench.hpp"
#include "geometry.hpp"

using namespace sme;

template <typename T>
static void geometry_Compartment_zero(benchmark::State &state) {
  T data;
  geometry::Compartment compartment;
  for (auto _ : state) {
    compartment = geometry::Compartment("id", data.img, 0);
  }
}

template <typename T>
static void geometry_Compartment(benchmark::State &state) {
  T data;
  geometry::Compartment compartment;
  for (auto _ : state) {
    compartment = geometry::Compartment("id", data.img, data.colours[0]);
  }
}

template <typename T> static void geometry_Membrane(benchmark::State &state) {
  T data;
  geometry::Membrane membrane;
  geometry::Compartment cA("cA", data.img, data.colours[0]);
  geometry::Compartment cB("cB", data.img, data.colours[1]);
  int p1{static_cast<int>(data.membraneIndexPair.first)};
  int p2{static_cast<int>(data.membraneIndexPair.second)};
  const auto *pointPairs{data.imageMembranePixels.getPoints(p1, p2)};
  const auto *weights{data.imageMembranePixels.getWeights(p1, p2)};
  for (auto _ : state) {
    membrane = geometry::Membrane("id", &cA, &cB, pointPairs, *weights);
  }
}

template <typename T> static void geometry_Field(benchmark::State &state) {
  T data;
  geometry::Compartment compartment("id", data.img, data.colours[0]);
  geometry::Field field;
  for (auto _ : state) {
    field = geometry::Field(&compartment, "id", 1.0, data.colours[0]);
  }
}

template <typename T>
static void geometry_Field_getConcentrationImageArray(benchmark::State &state) {
  T data;
  geometry::Compartment compartment("id", data.img, data.colours[0]);
  geometry::Field field(&compartment, "id", 1.0, data.colours[0]);
  field.setUniformConcentration(1.2);
  std::vector<double> concentration;
  for (auto _ : state) {
    concentration = field.getConcentrationImageArray();
  }
}

SME_BENCHMARK(geometry_Compartment_zero);
SME_BENCHMARK(geometry_Compartment);
SME_BENCHMARK(geometry_Membrane);
SME_BENCHMARK(geometry_Field);
SME_BENCHMARK(geometry_Field_getConcentrationImageArray);
