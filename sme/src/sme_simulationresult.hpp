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
  PyImageRgb concentrationImage;
  std::map<std::string, PyConc> speciesConcentration;
  std::string getStr() const;
  std::string getName() const;
};

} // namespace sme

PYBIND11_MAKE_OPAQUE(std::vector<sme::SimulationResult>);
