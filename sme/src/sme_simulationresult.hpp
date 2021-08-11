#pragma once

#include "sme_common.hpp"
#include <map>
#include <memory>
#include <pybind11/pybind11.h>
#include <string>
#include <vector>

namespace sme {

void pybindSimulationResult(pybind11::module &m);

struct SimulationResult {
  double timePoint{0.0};
  pybind11::array concentration_image{};
  pybind11::dict species_concentration{};
  pybind11::dict species_dcdt{};
  [[nodiscard]] std::string getStr() const;
  [[nodiscard]] std::string getName() const;
};

} // namespace sme

PYBIND11_MAKE_OPAQUE(std::vector<sme::SimulationResult>)
