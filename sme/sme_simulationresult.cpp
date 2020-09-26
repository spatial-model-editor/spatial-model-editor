// Python.h (included by pybind11.h) must come first:
#include <pybind11/pybind11.h>

// other headers
#include "sme_simulationresult.hpp"
#include <pybind11/stl.h>

namespace sme {

void pybindSimulationResult(const pybind11::module &m) {
  pybind11::class_<sme::SimulationResult>(m, "SimulationResult")
      .def_readonly("time_point", &sme::SimulationResult::timePoint,
                    "The timepoint these simulation results are from")
      .def_readonly("concentration_image",
                    &sme::SimulationResult::concentrationImage,
                    "An image of the species concentrations at this timepoint")
      .def_readonly("species_concentration",
                    &sme::SimulationResult::speciesConcentration,
                    "A dict of the species concentrations at this timepoint")
      .def("__repr__",
           [](const sme::SimulationResult &a) {
             return fmt::format("<sme.SimulationResult from timepoint {}>",
                                a.timePoint);
           })
      .def("__str__", &sme::SimulationResult::getStr);
}

std::string SimulationResult::getStr() const {
  std::string str("<sme.SimulationResult>\n");
  str.append(fmt::format("  - timepoint: {}\n", timePoint));
  str.append(
      fmt::format("  - number of species: {}\n", speciesConcentration.size()));
  return str;
}

} // namespace sme
