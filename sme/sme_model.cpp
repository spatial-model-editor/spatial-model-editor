#include "sme_model.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <QFile>
#include <exception>

#include "logger.hpp"
#include "sbml.hpp"
#include "sme_common.hpp"

namespace sme {

void pybindModel(pybind11::module& m) {
  pybind11::class_<sme::Model>(m, "Model")
      .def(pybind11::init<const std::string&>(), pybind11::arg("filename"))
      .def("export_sbml_file", &sme::Model::exportSBML,
           pybind11::arg("filename"))
      .def_property("name", &sme::Model::getName, &sme::Model::setName,
                    "The name of this model")
      .def_property_readonly("compartments", &sme::Model::getCompartments,
                             "The compartments in this model")
      .def("__repr__",
           [](const sme::Model& a) {
             return fmt::format("<sme.Model named '{}'>", a.getName());
           })
      .def("__str__", &sme::Model::getStr);
}

void Model::importSBML(const std::string& filename) {
  s = std::make_unique<sbml::SbmlDocWrapper>();
  QFile f(filename.c_str());
  if (f.open(QIODevice::ReadOnly)) {
    s->importSBMLString(f.readAll().toStdString());
  } else {
    throw std::runtime_error("Failed to open file: " + filename);
  }
  compartments.clear();
  compartments.reserve(static_cast<std::size_t>(s->compartments.size()));
  for (const auto& compartmentId : s->compartments) {
    compartments.emplace_back(s.get(), compartmentId.toStdString());
  }
}

Model::Model(const std::string& filename) {
  // disable logging
  spdlog::set_level(spdlog::level::off);
  // symengine assumes C locale
  std::locale::global(std::locale::classic());
  importSBML(filename);
}

void Model::exportSBML(const std::string& filename) {
  s->exportSBMLFile(filename);
}

void Model::setName(const std::string& name) { s->setName(name.c_str()); }

std::string Model::getName() const { return s->getName().toStdString(); }

std::map<std::string, Compartment*> Model::getCompartments() {
  return vecToNamePtrMap(compartments);
}

std::string Model::getStr() const {
  std::string str("<sme.Model>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(fmt::format("  - compartments:{}", vecToNames(compartments)));
  return str;
}

}  // namespace sme
