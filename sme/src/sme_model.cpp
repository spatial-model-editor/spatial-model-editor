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
                    R"(
                    CompartmentList: the compartments in this model
                    )")
      .def("compartment", &sme::Model::getCompartment, pybind11::arg("name"),
           R"(
           returns the compartment with the given name

           Args:
               name (str): The name of the compartment

           Returns:
               Compartment: the compartment if found.

           Raises:
               InvalidArgument: if no compartment was found with this name
           )")
      .def_readonly("membranes", &sme::Model::membranes,
                    R"(
                    MembraneList: the membranes in this model
                    )")
      .def("membrane", &sme::Model::getMembrane, pybind11::arg("name"),
           R"(
           returns the membrane with the given name.

           Args:
               name (str): The name of the membrane

           Returns:
               Membrane: the membrane if found.

           Raises:
               InvalidArgument: if no membrane was found with this name
           )")
      .def_readonly("parameters", &sme::Model::parameters,
                    R"(
                    ParameterList: the parameters in this model
                    )")
      .def("parameter", &sme::Model::getParameter, pybind11::arg("name"),
           R"(
           returns the parameter with the given name.

           Args:
               name (str): The name of the parameter

           Returns:
               Parameter: the parameter if found

           Raises:
               InvalidArgument: if no parameter was found with this name
           )")
      .def_readonly("compartment_image", &sme::Model::compartmentImage,
                    R"(
                    list of list of list of int: an image of the compartments in this model
                    )")
      .def("simulate", &sme::Model::simulate, pybind11::arg("simulation_time"),
           pybind11::arg("image_interval"),
           pybind11::arg("timeout_seconds") = 86400,
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
  QFile f(filename.c_str());
  if (f.open(QIODevice::ReadOnly)) {
    s = std::make_unique<model::Model>();
    s->importSBMLString(f.readAll().toStdString());
  } else {
    throw SmeInvalidArgument("Failed to open file: " + filename);
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

const Compartment &Model::getCompartment(const std::string &name) const {
  return findElem(compartments, name, "Compartment");
}

const Membrane &Model::getMembrane(const std::string &name) const {
  return findElem(membranes, name, "Membrane");
}

const Parameter &Model::getParameter(const std::string &name) const {
  return findElem(parameters, name, "Parameter");
}

static SimulationResult getSimulationResult(const simulate::Simulation *sim) {
  SimulationResult result;
  std::size_t i = sim->getTimePoints().size() - 1;
  result.timePoint = sim->getTimePoints()[i];
  result.concentrationImage = toPyImageRgb(sim->getConcImage(i, {}, true));
  result.speciesConcentration = sim->getPyConcs(i);
  return result;
}

std::vector<SimulationResult> Model::simulate(double simulationTime,
                                              double imageInterval,
                                              int timeoutSeconds) {
  QElapsedTimer simulationRuntimeTimer;
  simulationRuntimeTimer.start();
  std::vector<SimulationResult> results;
  sim = std::make_unique<simulate::Simulation>(*(s.get()),
                                               simulate::SimulatorType::Pixel);
  if (const auto &e = sim->errorMessage(); !e.empty()) {
    throw SmeRuntimeError(fmt::format("Error in simulation setup: {}", e));
  }
  results.push_back(getSimulationResult(sim.get()));
  while (sim->getTimePoints().back() < simulationTime) {
    sim->doTimestep(imageInterval);
    if (const auto &e = sim->errorMessage(); !e.empty()) {
      throw SmeRuntimeError(fmt::format("Error during simulation: {}", e));
    }
    if (auto secs = static_cast<int>(simulationRuntimeTimer.elapsed() / 1000);
        secs > timeoutSeconds) {
      throw SmeRuntimeError(fmt::format("Simulation timeout: {}s", secs));
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
