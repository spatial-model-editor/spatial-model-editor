// Python.h (included by pybind11.h) must come first:
#include <pybind11/pybind11.h>

// other headers
#include "logger.hpp"
#include "model.hpp"
#include "sme_common.hpp"
#include "sme_compartment.hpp"
#include <exception>
#include <pybind11/stl.h>

namespace sme {

void pybindCompartment(const pybind11::module &m) {
  pybind11::class_<sme::Compartment>(m, "Compartment")
      .def_property("name", &sme::Compartment::getName,
                    &sme::Compartment::setName, "The name of this compartment")
      .def_readonly("species", &sme::Compartment::species,
                    "The species in this compartment")
      .def("specie", &sme::Compartment::getSpecies, pybind11::arg("name"))
      .def_readonly("reactions", &sme::Compartment::reactions,
                    "The reactions in this compartment")
      .def("reaction", &sme::Compartment::getReaction, pybind11::arg("name"))
      .def("__repr__",
           [](const sme::Compartment &a) {
             return fmt::format("<sme.Compartment named '{}'>", a.getName());
           })
      .def("__str__", &sme::Compartment::getStr);
}

Compartment::Compartment(model::Model *sbmlDocWrapper, const std::string &sId)
    : s(sbmlDocWrapper), id(sId) {
  const auto &compSpecies = s->getSpecies().getIds(id.c_str());
  species.reserve(static_cast<std::size_t>(compSpecies.size()));
  for (const auto &spec : compSpecies) {
    species.emplace_back(s, spec.toStdString());
  }
  if (auto reacs = s->getReactions().getIds(sId.c_str()); !reacs.isEmpty()) {
    for (const auto &reac : reacs) {
      reactions.emplace_back(s, reac.toStdString());
    }
  }
}

std::string Compartment::getName() const {
  return s->getCompartments().getName(id.c_str()).toStdString();
}

void Compartment::setName(const std::string &name) {
  s->getCompartments().setName(id.c_str(), name.c_str());
}

const Species &Compartment::getSpecies(const std::string &name) const {
  return findElem(species, name, "Species");
}

const Reaction &Compartment::getReaction(const std::string &name) const {
  return findElem(reactions, name, "Reaction");
}

std::string Compartment::getStr() const {
  std::string str("<sme.Compartment>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(fmt::format("  - species: {}", vecToNames(species)));
  return str;
}

} // namespace sme
