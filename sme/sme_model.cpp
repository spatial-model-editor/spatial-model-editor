// Python.h (included by pybind11.h) must come first:
#include <pybind11/pybind11.h>

// other headers
#include "logger.hpp"
#include "sme_common.hpp"
#include "sme_model.hpp"
#include <QFile>
#include <QImage>
#include <exception>
#include <pybind11/stl.h>

namespace sme {

void pybindModel(const pybind11::module &m) {
  pybind11::class_<sme::Model>(m, "Model")
      .def(pybind11::init<const std::string &>(), pybind11::arg("filename"))
      .def_property("name", &sme::Model::getName, &sme::Model::setName,
                    "The name of this model")
      .def("export_sbml_file", &sme::Model::exportSbmlFile,
           pybind11::arg("filename"))
      .def_readonly("compartments", &sme::Model::compartments,
                    "The compartments in this model")
      .def("compartment", &sme::Model::getCompartment, pybind11::arg("name"))
      .def_readonly("parameters", &sme::Model::parameters,
                    "The parameters in this model")
      .def("parameter", &sme::Model::getParameter, pybind11::arg("name"))
      .def_readonly("compartment_image", &sme::Model::compartmentImage)
      .def("simulate", &sme::Model::simulate, pybind11::arg("simulation_time"),
           pybind11::arg("image_interval"))
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
    throw std::invalid_argument("Failed to open file: " + filename);
  }
  compartments.clear();
  compartments.reserve(
      static_cast<std::size_t>(s->getCompartments().getIds().size()));
  for (const auto &compartmentId : s->getCompartments().getIds()) {
    compartments.emplace_back(s.get(), compartmentId.toStdString());
  }
  parameters.clear();
  parameters.reserve(
      static_cast<std::size_t>(s->getParameters().getIds().size()));
  for (const auto &paramId : s->getParameters().getIds()) {
    parameters.emplace_back(s.get(), paramId.toStdString());
  }
  compartmentImage = toPyImage(s->getGeometry().getImage());
}

Model::Model(const std::string &filename) {
#if SPDLOG_ACTIVE_LEVEL > SPDLOG_LEVEL_TRACE
  // disable logging
  spdlog::set_level(spdlog::level::off);
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

const Parameter &Model::getParameter(const std::string &name) const {
  return findElem(parameters, name, "Parameter");
}

static SimulationResult getSimulationResult(const simulate::Simulation *sim) {
  SimulationResult result;
  std::size_t i = sim->getTimePoints().size() - 1;
  result.timePoint = sim->getTimePoints()[i];
  result.concentrationImage = toPyImage(sim->getConcImage(i, {}, true));
  result.speciesConcentration = sim->getPyConcs(i);
  return result;
}

std::vector<SimulationResult> Model::simulate(double simulationTime,
                                              double imageInterval) {
  std::vector<SimulationResult> results;
  sim = std::make_unique<simulate::Simulation>(*(s.get()),
                                               simulate::SimulatorType::Pixel);
  if (const auto &e = sim->errorMessage(); !e.empty()) {
    throw std::runtime_error(fmt::format("Error in simulation setup: {}", e));
  }
  results.push_back(getSimulationResult(sim.get()));
  while (sim->getTimePoints().back() < simulationTime) {
    sim->doTimestep(imageInterval);
    results.push_back(getSimulationResult(sim.get()));
  }
  return results;
}

std::string Model::getStr() const {
  std::string str("<sme.Model>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(fmt::format("  - compartments:{}", vecToNames(compartments)));
  return str;
}

} // namespace sme
