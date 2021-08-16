// Python.h (included by pybind11.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <pybind11/pybind11.h>

#include "logger.hpp"
#include "sme_common.hpp"
#include "sme_model.hpp"
#include "tiff.hpp"
#include <QElapsedTimer>
#include <QFile>
#include <QImage>

namespace sme {

void pybindModel(pybind11::module &m) {
  pybind11::class_<sme::Model> model(m, "Model",
                                     R"(
                                     the spatial model
                                     )");
  pybind11::enum_<simulate::SimulatorType>(m, "SimulatorType")
      .value("DUNE", simulate::SimulatorType::DUNE)
      .value("Pixel", simulate::SimulatorType::Pixel);
  model.def(pybind11::init<const std::string &>(), pybind11::arg("filename"))
      .def_property("name", &sme::Model::getName, &sme::Model::setName,
                    R"(
                    str: the name of this model
                    )")
      .def("export_sbml_file", &sme::Model::exportSbmlFile,
           pybind11::arg("filename"),
           R"(
           exports the model as a spatial SBML file

           Args:
               filename (str): the name of the file to create
           )")
      .def("export_sme_file", &sme::Model::exportSmeFile,
           pybind11::arg("filename"),
           R"(
           exports the model as a sme file

           Args:
               filename (str): the name of the file to create
           )")
      .def_readonly("compartments", &sme::Model::compartments,
                    pybind11::return_value_policy::reference_internal,
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
      .def_readonly("membranes", &sme::Model::membranes,
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
      .def_readonly("parameters", &sme::Model::parameters,
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
      .def_readonly("compartment_image", &sme::Model::compartmentImage,
                    R"(
                    list of list of list of int: an image of the compartments in this model
                    )")
      .def("import_geometry_from_image", &sme::Model::importGeometryFromImage,
           pybind11::arg("filename"),
           R"(
           sets the geometry of each compartment to the corresponding pixels in the supplied geometry image

           Note:
               Currently this function assumes that the compartments maintain the same colour
               as they had with the previous geometry image. If the new image does not contain
               pixels of each of these colours, the new model geometry will not be valid.
               The size of a pixel (in physical units) is also unchanged by this function.

           Args:
               filename (str): the name of the geometry image to import
           )")
      .def("simulate", &sme::Model::simulateFloat,
           pybind11::arg("simulation_time"), pybind11::arg("image_interval"),
           pybind11::arg("timeout_seconds") = 86400,
           pybind11::arg("throw_on_timeout") = true,
           pybind11::arg("simulator_type") = simulate::SimulatorType::Pixel,
           pybind11::arg("continue_existing_simulation") = false,
           pybind11::arg("return_results") = true,
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

           Returns:
               SimulationResultList: the results of the simulation

           Raises:
               RuntimeError: if the simulation times out or fails
           )")
      .def("simulate", &sme::Model::simulateString,
           pybind11::arg("simulation_times"), pybind11::arg("image_intervals"),
           pybind11::arg("timeout_seconds") = 86400,
           pybind11::arg("throw_on_timeout") = true,
           pybind11::arg("simulator_type") = simulate::SimulatorType::Pixel,
           pybind11::arg("continue_existing_simulation") = false,
           pybind11::arg("return_results") = true,
           R"(
           returns the results of the simulation.

           Args:
               simulation_times (str): The length(s) of the simulation in model units of time as a comma-delimited list, e.g. `"5"`, or `"10;100;20"`
               image_intervals (str): The interval(s) between images in model units of time as a comma-delimited list, e.g. `"1"`, or `"2;10;0.5"`
               timeout_seconds (int): The maximum time in seconds that the simulation can run for. Default value: 86400 = 1 day.
               throw_on_timeout (bool): Whether to throw an exception on simulation timeout. Default value: `true`.
               simulator_type (sme.SimulatorType): The simulator to use: `sme.SimulatorType.DUNE` or `sme.SimulatorType.Pixel`. Default value: Pixel.
               continue_existing_simulation (bool): Whether to continue the existing simulation, or start a new simulation. Default value: `false`, i.e. any existing simulation results are discarded before doing the simulation.
               return_results (bool): Whether to return the simulation results. Default value: `True`. If `False`, an empty SimulationResultList is returned.

           Returns:
               SimulationResultList: the results of the simulation

           Raises:
               RuntimeError: if the simulation times out or fails
           )")
      .def("__repr__",
           [](const sme::Model &a) {
             return fmt::format("<sme.Model named '{}'>", a.getName());
           })
      .def("__str__", &sme::Model::getStr);
}

static std::vector<SimulationResult>
getSimulationResults(const simulate::Simulation *sim) {
  std::vector<SimulationResult> results;
  for (std::size_t i = 0; i < sim->getTimePoints().size(); ++i) {
    auto &result = results.emplace_back();
    result.timePoint = sim->getTimePoints()[i];
    result.concentrationImage = toPyImageRgb(sim->getConcImage(i, {}, true));
    std::tie(result.speciesConcentration, result.speciesDcdt) =
        sim->getPyConcs(i);
  }
  return results;
}

void Model::init() {
  if (!s->getIsValid()) {
    throw SmeInvalidArgument("Failed to open model: " +
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
  compartmentImage = toPyImageRgb(s->getGeometry().getImage());
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
  s = std::make_unique<model::Model>();
  s->importFile(filename);
  init();
}

void Model::importSbmlString(const std::string &xml) {
  s = std::make_unique<model::Model>();
  s->importSBMLString(xml);
  init();
}

std::string Model::getName() const { return s->getName().toStdString(); }

void Model::setName(const std::string &name) { s->setName(name.c_str()); }

void Model::importGeometryFromImage(const std::string &filename) {
  // store existing compartment colours
  auto ids{s->getCompartments().getIds()};
  auto colours{s->getCompartments().getColours()};
  // import geometry image
  QImage img;
  sme::utils::TiffReader tiffReader(filename);
  if (tiffReader.size() > 0) {
    img = tiffReader.getImage();
  } else {
    img = QImage(filename.c_str());
  }
  // this resets all compartment colours
  s->getGeometry().importGeometryFromImage(img);
  compartmentImage = toPyImageRgb(s->getGeometry().getImage());
  // try to re-assign previous colour to each compartment
  for (int i = 0; i < ids.size(); ++i) {
    s->getCompartments().setColour(ids[i], colours[i]);
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
                      simulate::SimulatorType simulatorType,
                      bool continueExistingSimulation, bool returnResults) {
  QElapsedTimer simulationRuntimeTimer;
  simulationRuntimeTimer.start();
  double timeoutMillisecs{static_cast<double>(timeoutSeconds) * 1000.0};
  if (!continueExistingSimulation) {
    s->getSimulationData().clear();
  }
  s->getSimulationSettings().simulatorType = simulatorType;
  auto times{
      simulate::parseSimulationTimes(lengths.c_str(), intervals.c_str())};
  if (!times.has_value()) {
    throw SmeRuntimeError("Invalid simulation lengths or intervals");
  }
  // ensure any existing DUNE objects are destroyed to avoid later segfaults
  sim.reset();
  sim = std::make_unique<simulate::Simulation>(*(s.get()));
  if (const auto &e = sim->errorMessage(); !e.empty()) {
    throw SmeRuntimeError(fmt::format("Error in simulation setup: {}", e));
  }
  sim->doMultipleTimesteps(times.value(), timeoutMillisecs);
  if (const auto &e = sim->errorMessage(); throwOnTimeout && !e.empty()) {
    throw SmeRuntimeError(fmt::format("Error during simulation: {}", e));
  }
  if (returnResults) {
    return getSimulationResults(sim.get());
  }
  return {};
}

std::vector<SimulationResult>
Model::simulateFloat(double simulationTime, double imageInterval,
                     int timeoutSeconds, bool throwOnTimeout,
                     simulate::SimulatorType simulatorType,
                     bool continueExistingSimulation, bool returnResults) {
  return simulateString(QString::number(simulationTime, 'g', 17).toStdString(),
                        QString::number(imageInterval, 'g', 17).toStdString(),
                        timeoutSeconds, throwOnTimeout, simulatorType,
                        continueExistingSimulation, returnResults);
}

std::string Model::getStr() const {
  std::string str("<sme.Model>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(fmt::format("  - compartments:{}\n", vecToNames(compartments)));
  str.append(fmt::format("  - membranes:{}", vecToNames(membranes)));
  return str;
}

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
