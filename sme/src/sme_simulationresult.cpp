// Python.h (included by pybind11.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <pybind11/pybind11.h>

#include "sme_simulationresult.hpp"

namespace sme {

void pybindSimulationResult(pybind11::module &m) {
  sme::bindList<SimulationResult>(m, "SimulationResult");
  pybind11::class_<SimulationResult>(m, "SimulationResult",
                                     R"(
                                     results at a single timepoint of a simulation
                                     )")
      .def_readonly("time_point", &SimulationResult::timePoint,
                    R"(
                    float: the timepoint these simulation results are from
                    )")
      .def_readonly("concentration_image",
                    &SimulationResult::concentrationImage,
                    R"(
                    list of list of list of int: an image of the species concentrations at this timepoint

                    a triplet of red, green, blue values for each pixel in the image
                    concentration_image[y][x] = [r, g, b]
                    )")
      .def_readonly("species_concentration",
                    &SimulationResult::speciesConcentration,
                    R"(
                    dict: the species concentrations at this timepoint

                    for each species, the concentrations are provided as a
                    list of list of float, where species_concentration['A'][y][x]
                    is the concentration of species 'A" at the point (x,y)
                    )")
      .def_readonly("species_dcdt", &SimulationResult::speciesDcdt,
                    R"(
                    dict: the rate of change of the species concentrations at this timepoint

                    for each species, the rate of change of the concentrations are provided as a
                    list of list of float, where species_dcdt['A'][y][x] is the rate of change
                    of concentration of species 'A' at the point (x,y)
                    )")
      .def("__repr__",
           [](const SimulationResult &a) {
             return fmt::format("<sme.SimulationResult from timepoint {}>",
                                a.timePoint);
           })
      .def("__str__", &SimulationResult::getStr);
}

std::string SimulationResult::getStr() const {
  std::string str("<sme.SimulationResult>\n");
  str.append(fmt::format("  - timepoint: {}\n", timePoint));
  str.append(
      fmt::format("  - number of species: {}\n", speciesConcentration.size()));
  return str;
}

std::string SimulationResult::getName() const { return {}; }

} // namespace sme

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//
