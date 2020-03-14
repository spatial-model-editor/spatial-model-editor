#include "sme_compartment.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "sbml.hpp"
#include "sme_common.hpp"

namespace sme {

void pybindCompartment(pybind11::module& m) {
  pybind11::class_<sme::Compartment>(m, "Compartment")
      .def_property("name", &sme::Compartment::getName,
                    &sme::Compartment::setName, "The name of this compartment")
      .def_property_readonly("species", &sme::Compartment::getSpecies,
                             "The species in this compartment")
      .def("__repr__",
           [](const sme::Compartment& a) {
             return fmt::format("<sme.Compartment named '{}'>", a.getName());
           })
      .def("__str__", &sme::Compartment::getStr);
}

Compartment::Compartment(sbml::SbmlDocWrapper* sbmlDocWrapper,
                         const std::string& compartmentId)
    : s(sbmlDocWrapper), id(compartmentId) {
  const auto& compSpecies = s->species.at(id.c_str());
  species.reserve(static_cast<std::size_t>(compSpecies.size()));
  for (const auto& spec : compSpecies) {
    species.emplace_back(s, spec.toStdString());
  }
}

const std::string& Compartment::getId() const { return id; }

void Compartment::setName(const std::string& name) {
  s->setCompartmentName(id.c_str(), name.c_str());
}

std::string Compartment::getName() const {
  return s->getCompartmentName(id.c_str()).toStdString();
}

std::map<std::string, Species*> Compartment::getSpecies() {
  return vecToNamePtrMap(species);
}

std::string Compartment::getStr() const {
  std::string str("<sme.Compartment>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(fmt::format("  - species: {}", vecToNames(species)));
  return str;
}

}  // namespace sme
