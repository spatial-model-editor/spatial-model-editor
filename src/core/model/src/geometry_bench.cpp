#include "bench.hpp"
#include "geometry.hpp"

template <typename T>
static void geometry_Compartment(benchmark::State &state) {
  T data;
  geometry::Compartment compartment;
  for (auto _ : state) {
    compartment = geometry::Compartment("id", data.img, data.colours[0]);
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

SME_BENCHMARK(geometry_Compartment);
SME_BENCHMARK(geometry_Field_getConcentrationImageArray);
