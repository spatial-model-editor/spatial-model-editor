#pragma once

#include "sme_common.hpp"
#include <map>
#include <memory>
#include <nanobind/nanobind.h>
#include <string>
#include <vector>

namespace pysme {

void bindSimulationResult(nanobind::module_ &m);

struct SimulationResult {
  double timePoint{0.0};
  nanobind::ndarray<nanobind::numpy, std::uint8_t> concentration_image{};
  nanobind::dict species_concentration{};
  nanobind::dict species_dcdt{};
  [[nodiscard]] std::string getStr() const;
  [[nodiscard]] std::string getName() const;
};

} // namespace pysme

NB_MAKE_OPAQUE(std::vector<pysme::SimulationResult>)
