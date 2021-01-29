// Python.h (included by pybind11.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <pybind11/pybind11.h>

#include "logger.hpp"
#include "sme_common.hpp"
#include "sme_model.hpp"
#include <QElapsedTimer>
#include <QFile>
#include <QImage>

namespace sme {

void pybindModel(pybind11::module &m) {
  pybind11::class_<sme::Model>(m, "Model",
                               R"(
                               the spatial model
                               )")
      .def(pybind11::init<const std::string &>(), pybind11::arg("filename"))
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
      .def("simulate", &sme::Model::simulate, pybind11::arg("simulation_time"),
           pybind11::arg("image_interval"),
           pybind11::arg("timeout_seconds") = 86400,
           pybind11::arg("throw_on_timeout") = true,
           R"(
           returns the results of the simulation.

           Args:
               simulation_time (float): The length of the simulation in model units of time
               image_interval (float): The interval between images in model units of time
               timeout_seconds (int): The maximum time in seconds that the simulation can run for

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

void Model::importSbmlFile(const std::string &filename) {
  std::string xml;
  if (QFile f(filename.c_str());
      f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    xml = f.readAll().toStdString();
  } else {
    throw SmeInvalidArgument("Failed to open file: " + filename);
  }
  s = std::make_unique<model::Model>();
  s->importSBMLString(xml);
  if (!s->getIsValid()) {
    throw SmeInvalidArgument("Failed to import SBML from file: " + filename);
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
  importSbmlFile(filename);
}

std::string Model::getName() const { return s->getName().toStdString(); }

void Model::setName(const std::string &name) { s->setName(name.c_str()); }

void Model::exportSbmlFile(const std::string &filename) {
  s->exportSBMLFile(filename);
}

static SimulationResult getSimulationResult(const simulate::Simulation *sim) {
  SimulationResult result;
  std::size_t i = sim->getTimePoints().size() - 1;
  result.timePoint = sim->getTimePoints()[i];
  result.concentrationImage = toPyImageRgb(sim->getConcImage(i, {}, true));
  std::tie(result.speciesConcentration, result.speciesDcdt) =
      sim->getPyConcs(i);
  return result;
}

std::vector<SimulationResult> Model::simulate(double simulationTime,
                                              double imageInterval,
                                              int timeoutSeconds,
                                              bool throwOnTimeout) {
  QElapsedTimer simulationRuntimeTimer;
  simulationRuntimeTimer.start();
  double timeoutMillisecs{static_cast<double>(timeoutSeconds) * 1000.0};
  std::vector<SimulationResult> results;
  sim = std::make_unique<simulate::Simulation>(*(s.get()),
                                               simulate::SimulatorType::Pixel);
  if (const auto &e = sim->errorMessage(); !e.empty()) {
    throw SmeRuntimeError(fmt::format("Error in simulation setup: {}", e));
  }
  results.push_back(getSimulationResult(sim.get()));
  while (sim->getTimePoints().back() < simulationTime) {
    double remainingTimeoutMillisecs{
        timeoutMillisecs -
        static_cast<double>(simulationRuntimeTimer.elapsed())};
    if(remainingTimeoutMillisecs < 0){
      remainingTimeoutMillisecs = 0.0;
    }
    sim->doTimesteps(imageInterval, 1, remainingTimeoutMillisecs);
    if (const auto &e = sim->errorMessage(); !e.empty()) {
      if(throwOnTimeout) {
        throw SmeRuntimeError(fmt::format("Error during simulation: {}", e));
      } else {
        return results;
      }
    }
    results.push_back(getSimulationResult(sim.get()));
  }
  return results;
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
