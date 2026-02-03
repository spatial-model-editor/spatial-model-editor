// Python.h (#included by nanobind.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <nanobind/nanobind.h>

#include "sme/image_stack.hpp"
#include "sme/logger.hpp"
#include "sme_common.hpp"
#include "sme_model.hpp"
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <nanobind/stl/string.h>

namespace pysme {

void bindModel(nanobind::module_ &m) {
  nanobind::class_<Model> model(m, "Model",
                                R"(
                                     the spatial model
                                     )");
  nanobind::enum_<::sme::simulate::SimulatorType>(m, "SimulatorType")
      .value("DUNE", ::sme::simulate::SimulatorType::DUNE)
      .value("Pixel", ::sme::simulate::SimulatorType::Pixel);
  model.def(nanobind::init<const std::string &>(), nanobind::arg("filename"))
      .def_prop_rw("name", &Model::getName, &Model::setName,
                   R"(
                    str: the name of this model
                    )")
      .def("export_sbml_file", &Model::exportSbmlFile,
           nanobind::arg("filename"),
           R"(
           exports the model as a spatial SBML file

           Args:
               filename (str): the name of the file to create
           )")
      .def("export_sme_file", &Model::exportSmeFile, nanobind::arg("filename"),
           R"(
           exports the model as a sme file

           Args:
               filename (str): the name of the file to create
           )")
      .def_ro("compartments", &Model::compartments,
              nanobind::rv_policy::reference_internal,
              R"(
                    CompartmentList: the compartments in this model

                    a list of :class:`Compartment` that can be iterated over,
                    or indexed into by name or position in the list.

                    Examples:
                        the list of compartments can be iterated over:

                        >>> import sme
                        >>> model = sme.open_example_model()
                        >>> for compartment in model.compartments:
                        ...     print(compartment.name)
                        Outside
                        Cell
                        Nucleus

                        or a compartment can be found using its name:

                        >>> cell = model.compartments["Cell"]
                        >>> print(cell.name)
                        Cell

                        or indexed by its position in the list:

                        >>> last_compartment = model.compartments[-1]
                        >>> print(last_compartment.name)
                        Nucleus
           )")
      .def_ro("membranes", &Model::membranes,
              R"(
                    MembraneList: the membranes in this model

                    a list of :class:`Membrane` that can be iterated over,
                    or indexed into by name or position in the list.

                    Examples:
                        the list of membranes can be iterated over:

                        >>> import sme
                        >>> model = sme.open_example_model()
                        >>> for membrane in model.membranes:
                        ...     print(membrane.name)
                        Outside <-> Cell
                        Cell <-> Nucleus

                        or a membrane can be found using its name:

                        >>> outer = model.membranes["Outside <-> Cell"]
                        >>> print(outer.name)
                        Outside <-> Cell

                        or indexed by its position in the list:

                        >>> last_membrane = model.membranes[-1]
                        >>> print(last_membrane.name)
                        Cell <-> Nucleus
                    )")
      .def_ro("parameters", &Model::parameters,
              R"(
                    ParameterList: the parameters in this model

                    a list of :class:`Parameter` that can be iterated over,
                    or indexed into by name or position in the list.

                    Examples:
                        the list of parameters can be iterated over:

                        >>> import sme
                        >>> model = sme.open_example_model()
                        >>> for parameter in model.parameters:
                        ...     print(parameter.name)
                        param

                        or a parameter can be found using its name:

                        >>> p = model.parameters["param"]
                        >>> print(p.name)
                        param

                        or indexed by its position in the list:

                        >>> last_param = model.parameters[-1]
                        >>> print(last_param.name)
                        param
                    )")
      .def_prop_ro("compartment_image", &Model::compartment_image,
                   nanobind::rv_policy::take_ownership,
                   R"(
                    numpy.ndarray: an image of the compartments in this model

                    An array of RGB integer values for each voxel in the image of
                    the compartments in this model,
                    which can be displayed using e.g. ``matplotlib.pyplot.imshow``

                    Examples:
                        the image is a 4d (depth x height x width x 3) array of integers:

                        >>> import sme
                        >>> model = sme.open_example_model()
                        >>> type(model.compartment_image)
                        <class 'numpy.ndarray'>
                        >>> model.compartment_image.dtype
                        dtype('uint8')
                        >>> model.compartment_image.shape
                        (1, 100, 100, 3)

                        each voxel in the image has a triplet of RGB integer values
                        in the range 0-255:

                        >>> model.compartment_image[0, 34, 36]
                        array([144,  97, 193], dtype=uint8)

                        the first z-slice of the image can be displayed using matplotlib:

                        >>> import matplotlib.pyplot as plt
                        >>> imgplot = plt.imshow(model.compartment_image[0]))")
      .def("import_geometry_from_image", &Model::importGeometryFromImage,
           nanobind::arg("filename"),
           R"(
           sets the geometry of each compartment to the corresponding pixels in the supplied geometry image

           Note:
               Currently this function assumes that the compartments maintain the same color
               as they had with the previous geometry image. If the new image does not contain
               pixels of each of these colors, the new model geometry will not be valid.
               The volume of a pixel (in physical units) is also unchanged by this function.

           Args:
               filename (str): the name of the geometry image to import
           )")
      .def("simulate", &Model::simulateFloat, nanobind::arg("simulation_time"),
           nanobind::arg("image_interval"),
           nanobind::arg("timeout_seconds") = 86400,
           nanobind::arg("throw_on_timeout") = true,
           nanobind::arg("simulator_type") =
               ::sme::simulate::SimulatorType::Pixel,
           nanobind::arg("continue_existing_simulation") = false,
           nanobind::arg("return_results") = true,
           nanobind::arg("n_threads") = 1,
           R"(
           returns the results of the simulation.

           Args:
               simulation_time (float): The length of the simulation in model units of time, e.g. `5.5`
               image_interval (float): The interval between images in model units of time, e.g. `1.1`
               timeout_seconds (int): The maximum time in seconds that the simulation can run for. Default value: 86400 = 1 day.
               throw_on_timeout (bool): Whether to throw an exception on simulation timeout. Default value: `True`.
               simulator_type (sme.SimulatorType): The simulator to use: `sme.SimulatorType.DUNE` or `sme.SimulatorType.Pixel`. Default value: Pixel.
               continue_existing_simulation (bool): Whether to continue the existing simulation, or start a new simulation. Default value: `False`, i.e. any existing simulation results are discarded before doing the simulation.
               return_results (bool): Whether to return the simulation results. Default value: `True`. If `False`, an empty SimulationResultList is returned.
               n_threads(int): Number of cpu threads to use (for Pixel simulations). Default value is 1, 0 means use all available threads.

           Returns:
               SimulationResultList: the results of the simulation

           Raises:
               RuntimeError: if the simulation times out or fails
           )")
      .def("simulate", &Model::simulateString,
           nanobind::arg("simulation_times"), nanobind::arg("image_intervals"),
           nanobind::arg("timeout_seconds") = 86400,
           nanobind::arg("throw_on_timeout") = true,
           nanobind::arg("simulator_type") =
               ::sme::simulate::SimulatorType::Pixel,
           nanobind::arg("continue_existing_simulation") = false,
           nanobind::arg("return_results") = true,
           nanobind::arg("n_threads") = 1,
           R"(
           returns the results of the simulation.

           Args:
               simulation_times (str): The length(s) of the simulation in model units of time as a semicolon-delimited list, e.g. `"5"`, or `"10;100;20"`
               image_intervals (str): The interval(s) between images in model units of time as a semicolon-delimited list, e.g. `"1"`, or `"2;10;0.5"`
               timeout_seconds (int): The maximum time in seconds that the simulation can run for. Default value: 86400 = 1 day.
               throw_on_timeout (bool): Whether to throw an exception on simulation timeout. Default value: `true`.
               simulator_type (sme.SimulatorType): The simulator to use: `sme.SimulatorType.DUNE` or `sme.SimulatorType.Pixel`. Default value: Pixel.
               continue_existing_simulation (bool): Whether to continue the existing simulation, or start a new simulation. Default value: `false`, i.e. any existing simulation results are discarded before doing the simulation.
               return_results (bool): Whether to return the simulation results. Default value: `True`. If `False`, an empty SimulationResultList is returned.
               n_threads(int): Number of cpu threads to use (for Pixel simulations). Default value is 1, 0 means use all available threads.

           Returns:
               SimulationResultList: the results of the simulation

           Raises:
               RuntimeError: if the simulation times out or fails
           )")
      .def("simulation_results", &Model::getSimulationResults,
           R"(
          returns the simulation results.

          Returns:
              SimulationResultList: the simulation results
          )")
      .def("__repr__",
           [](const Model &a) {
             return fmt::format("<sme.Model named '{}'>", a.getName());
           })
      .def("__str__", &Model::getStr);
}

static std::vector<SimulationResult>
constructSimulationResults(const ::sme::simulate::Simulation *sim,
                           bool getDcdt) {
  std::vector<SimulationResult> results;
  const auto timePoints{sim->getTimePoints()};
  results.reserve(timePoints.size());
  for (std::size_t i = 0; i < timePoints.size(); ++i) {
    auto &result = results.emplace_back();
    result.timePoint = timePoints[i];
    auto img = sim->getConcImage(i, {}, true);
    auto shape = img.volume();
    result.concentration_image = toPyImageRgb(img);
    for (std::size_t ci = 0; ci < sim->getCompartmentIds().size(); ++ci) {
      const auto &names{sim->getPyNames(ci)};
      auto concs{sim->getPyConcs(i, ci)};
      for (std::size_t si = 0; si < names.size(); ++si) {
        result.species_concentration[nanobind::str(names[si].data(),
                                                   names[si].size())] =
            as_ndarray(std::move(concs[si]), shape);
      }
      if (getDcdt && i + 1 == timePoints.size()) {
        if (auto dcdts{sim->getPyDcdts(ci)}; !dcdts.empty()) {
          for (std::size_t si = 0; si < names.size(); ++si) {
            result.species_dcdt[nanobind::str(names[si].data(),
                                              names[si].size())] =
                as_ndarray(std::move(dcdts[si]), shape);
          }
        }
      }
    }
  }
  return results;
}

void Model::init() {
  if (!s->getIsValid()) {
    throw std::invalid_argument("Failed to open model: " +
                                s->getErrorMessage().toStdString());
  }
  compartments.clear();
  compartments.reserve(
      static_cast<std::size_t>(s->getCompartments().getIds().size()));
  for (const auto &compartmentId : s->getCompartments().getIds()) {
    compartments.emplace_back(s.get(), compartmentId.toStdString());
  }
  membranes.clear();
  membranes.reserve(
      static_cast<std::size_t>(s->getMembranes().getIds().size()));
  for (const auto &membraneId : s->getMembranes().getIds()) {
    membranes.emplace_back(s.get(), membraneId.toStdString());
  }
  parameters.clear();
  parameters.reserve(
      static_cast<std::size_t>(s->getParameters().getIds().size()));
  for (const auto &paramId : s->getParameters().getIds()) {
    parameters.emplace_back(s.get(), paramId.toStdString());
  }
}

Model::Model(const std::string &filename) {
#if SPDLOG_ACTIVE_LEVEL > SPDLOG_LEVEL_TRACE
  // disable logging
  spdlog::set_level(spdlog::level::critical);
#endif
  if (!filename.empty()) {
    importFile(filename);
  }
}

void Model::importFile(const std::string &filename) {
  s = std::make_unique<::sme::model::Model>();
  s->importFile(filename);
  init();
}

void Model::importSbmlString(const std::string &xml) {
  s = std::make_unique<::sme::model::Model>();
  s->importSBMLString(xml);
  init();
}

std::string Model::getName() const { return s->getName().toStdString(); }

void Model::setName(const std::string &name) { s->setName(name.c_str()); }

nanobind::ndarray<nanobind::numpy, std::uint8_t>
Model::compartment_image() const {
  return toPyImageRgb(s->getGeometry().getImages());
}

void Model::importGeometryFromImage(const std::string &filename) {
  try {
    s->getGeometry().importGeometryFromImages(
        ::sme::common::ImageStack(filename.c_str()), true);
  } catch (const std::invalid_argument &e) {
    throw std::invalid_argument("Failed to import geometry from image '" +
                                filename + "': " + e.what());
  }
}

void Model::exportSbmlFile(const std::string &filename) {
  s->exportSBMLFile(filename);
}

void Model::exportSmeFile(const std::string &filename) {
  s->exportSMEFile(filename);
}

std::vector<SimulationResult>
Model::simulateString(const std::string &lengths, const std::string &intervals,
                      int timeoutSeconds, bool throwOnTimeout,
                      ::sme::simulate::SimulatorType simulatorType,
                      bool continueExistingSimulation, bool returnResults,
                      int nThreads) {
  QElapsedTimer simulationRuntimeTimer;
  simulationRuntimeTimer.start();
  double timeoutMillisecs{static_cast<double>(timeoutSeconds) * 1000.0};
  if (!continueExistingSimulation) {
    s->getSimulationData().clear();
  }
  s->getSimulationSettings().simulatorType = simulatorType;
  if (simulatorType == ::sme::simulate::SimulatorType::Pixel) {
    auto &pixelOpts{s->getSimulationSettings().options.pixel};
    if (nThreads != 1) {
      pixelOpts.enableMultiThreading = true;
      pixelOpts.maxThreads = static_cast<std::size_t>(nThreads);
    } else {
      pixelOpts.enableMultiThreading = false;
    }
  }
  auto times{::sme::simulate::parseSimulationTimes(lengths.c_str(),
                                                   intervals.c_str())};
  if (!times.has_value()) {
    throw std::invalid_argument("Invalid simulation lengths or intervals");
  }
  // ensure any existing DUNE objects are destroyed to avoid later segfaults
  sim.reset();
  sim = std::make_unique<::sme::simulate::Simulation>(*(s.get()));
  if (const auto &e = sim->errorMessage(); !e.empty()) {
    throw std::runtime_error(fmt::format("Error in simulation setup: {}", e));
  }
  sim->doMultipleTimesteps(times.value(), timeoutMillisecs, []() {
    if (PyErr_CheckSignals() != 0) {
      throw nanobind::python_error();
    }
    return false;
  });
  if (const auto &e = sim->errorMessage(); throwOnTimeout && !e.empty()) {
    throw std::runtime_error(fmt::format("Error during simulation: {}", e));
  }
  if (returnResults) {
    return constructSimulationResults(sim.get(), true);
  }
  return {};
}

std::vector<SimulationResult> Model::simulateFloat(
    double simulationTime, double imageInterval, int timeoutSeconds,
    bool throwOnTimeout, ::sme::simulate::SimulatorType simulatorType,
    bool continueExistingSimulation, bool returnResults, int nThreads) {
  return simulateString(QString::number(simulationTime, 'g', 17).toStdString(),
                        QString::number(imageInterval, 'g', 17).toStdString(),
                        timeoutSeconds, throwOnTimeout, simulatorType,
                        continueExistingSimulation, returnResults, nThreads);
}

std::vector<SimulationResult> Model::getSimulationResults() {
  sim.reset();
  sim = std::make_unique<::sme::simulate::Simulation>(*s);
  if (const auto &e{sim->errorMessage()}; !e.empty()) {
    throw std::runtime_error(fmt::format("Error in simulation setup: {}", e));
  }
  return constructSimulationResults(sim.get(), false);
}

std::string Model::getStr() const {
  std::string str("<sme.Model>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(fmt::format("  - compartments:{}\n", vecToNames(compartments)));
  str.append(fmt::format("  - membranes:{}", vecToNames(membranes)));
  return str;
}

} // namespace pysme
