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
                    &SimulationResult::concentration_image,
                    R"(
                    numpy.ndarray: an image of the species concentrations at this timepoint

                    An array of RGB integer values for each pixel in the image of
                    the compartments in this model,
                    which can be displayed using e.g. ``matplotlib.pyplot.imshow``

                    Examples:

                        do a short simulation and get the concentration image from the last timepoint:

                        >>> import sme
                        >>> model = sme.open_example_model()
                        >>> results = model.simulate(10, 1)
                        >>> concentration_image = results[-1].concentration_image

                        the image is a 3d (height x width x 3) array of integers:

                        >>> type(concentration_image)
                        <class 'numpy.ndarray'>
                        >>> concentration_image.dtype
                        dtype('uint8')
                        >>> concentration_image.shape
                        (100, 100, 3)

                        each pixel in the image has a triplet of RGB integer values
                        in the range 0-255:

                        >>> concentration_image[34, 36]
                        array([33, 23,  9], dtype=uint8)

                        the image can be displayed using matplotlib:

                        >>> import matplotlib.pyplot as plt
                        >>> imgplot = plt.imshow(concentration_image)
                    )")
      .def_readonly("species_concentration",
                    &SimulationResult::species_concentration,
                    R"(
                    Dict[str, numpy.ndarray]: the species concentrations at this timepoint

                    for each species, the concentrations are provided as a
                    2d array, where ``species_concentration['A'][y][x]``
                    is the concentration of species "A" at the point (x,y)

                    Examples:
                        do a short simulation and get the species concentrations from the last timepoint:

                        >>> import sme
                        >>> model = sme.open_example_model()
                        >>> results = model.simulate(10, 1)
                        >>> species_concentration = results[-1].species_concentration

                        this is a dict with an entry for each species:

                        >>> type(species_concentration)
                        <class 'dict'>
                        >>> species_concentration.keys()
                        dict_keys(['B_out', 'A_cell', 'B_cell', 'A_nucl', 'B_nucl'])

                        the concentrations are a 2d ndarray of doubles,
                        one for each pixel in the geometry image:

                        >>> b_cell = species_concentration['B_cell']
                        >>> type(b_cell)
                        <class 'numpy.ndarray'>
                        >>> b_cell.dtype
                        dtype('float64')
                        >>> b_cell.shape
                        (100, 100)

                        the concentrations can be displayed using matplotlib:

                        >>> import matplotlib.pyplot as plt
                        >>> imgplot = plt.imshow(b_cell)
                    )")
      .def_readonly("species_dcdt", &SimulationResult::species_dcdt,
                    R"(
                    Dict[str, numpy.ndarray]: the species concentration rate of change at this timepoint

                    for each species, the rate of change of concentration is provided as a
                    2d array, where ``species_dcdt['A'][y][x]``
                    is the rate of change of the concentration of species "A" at the point (x,y)

                    Note:
                        The rate of change of species concentrations is only provided
                        for the last timepoint of a simulation, and only when using
                        the Pixel simulator. Otherwise ``species_dcdt`` is an empty dict.

                    Examples:
                        do a short Pixel simulation and get the rate of change of
                        species concentrations from the last timepoint:

                        >>> import sme
                        >>> model = sme.open_example_model()
                        >>> results = model.simulate(10, 1)
                        >>> species_dcdt = results[-1].species_dcdt

                        this is a dict with an entry for each species:

                        >>> type(species_dcdt)
                        <class 'dict'>
                        >>> species_dcdt.keys()
                        dict_keys(['B_out', 'A_cell', 'B_cell', 'A_nucl', 'B_nucl'])

                        the rate of change of concentration is a 2d ndarray of doubles,
                        one for each pixel in the geometry image:

                        >>> b_cell = species_dcdt['B_cell']
                        >>> type(b_cell)
                        <class 'numpy.ndarray'>
                        >>> b_cell.dtype
                        dtype('float64')
                        >>> b_cell.shape
                        (100, 100)

                        the rate of change of concentration can be displayed using matplotlib:

                        >>> import matplotlib.pyplot as plt
                        >>> imgplot = plt.imshow(b_cell)
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
      fmt::format("  - number of species: {}\n", species_concentration.size()));
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
