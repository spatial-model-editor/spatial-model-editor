#pragma once

#include "sme_common.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace pybind11 {
class module;
}

namespace sme {

void pybindSimulationResult(const pybind11::module &m);

struct SimulationResult {
  double timePoint{0.0};
  PyImage concentrationImage;
  std::map<std::string, PyConc> speciesConcentration;
  std::string getStr() const;
};

} // namespace sme
