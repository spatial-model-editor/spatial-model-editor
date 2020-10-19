// Python.h (included by pybind11.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <pybind11/pybind11.h>

#include "logger.hpp"
#include "model.hpp"
#include "sme_compartment.hpp"
#include <pybind11/stl_bind.h>

namespace sme {

void pybindCompartment(pybind11::module &m) {
  pybind11::bind_vector<std::vector<Compartment>>(m, "CompartmentList",
                                                  R"(
                                                  a list of compartments
                                                  )");
  pybind11::class_<Compartment>(m, "Compartment",
                                R"(
                                a compartment where species live
                                )")
      .def_property("name", &Compartment::getName, &Compartment::setName,
                    R"(
                    str: the name of this compartment
                    )")
      .def_readonly("species", &Compartment::species,
                    R"(
                    SpeciesList: the species in this compartment
                    )")
      .def("specie", &Compartment::getSpecies, pybind11::arg("name"),
           R"(
           Returns the species with the given name.

           Args:
               name (str): The name of the species

           Returns:
               Species: the species if found.

           Raises:
               InvalidArgument: if no species was found with this name
           )")
      .def_readonly("reactions", &Compartment::reactions,
                    R"(
                    ReactionList: the reactions in this compartment
                    )")
      .def("reaction", &Compartment::getReaction, pybind11::arg("name"),
           R"(
           Returns the reaction with the given name.

           Args:
               name (str): The name of the reaction

           Returns:
               Reaction: the reaction if found.

           Raises:
               InvalidArgument: if no reaction was found with this name
           )")
      .def_readonly("geometry_mask", &Compartment::geometryMask,
                    R"(
                    list of list of bool: 2d pixel mask of the compartment geometry

                    The mask is a list of list of bool, where
                    ``geometry_mask[y][x] = True``
                    if the pixel at point (x,y) is part of this compartment
                    )")
      .def("__repr__",
           [](const Compartment &a) {
             return fmt::format("<sme.Compartment named '{}'>", a.getName());
           })
      .def("__str__", &Compartment::getStr);
}

Compartment::Compartment(model::Model *sbmlDocWrapper, const std::string &sId)
    : s(sbmlDocWrapper), id(sId) {
  const auto &compSpecies = s->getSpecies().getIds(id.c_str());
  species.reserve(static_cast<std::size_t>(compSpecies.size()));
  for (const auto &spec : compSpecies) {
    species.emplace_back(s, spec.toStdString());
  }
  if (auto reacs = s->getReactions().getIds(id.c_str()); !reacs.isEmpty()) {
    for (const auto &reac : reacs) {
      reactions.emplace_back(s, reac.toStdString());
    }
  }
  geometryMask = toPyImageMask(
      s->getCompartments().getCompartment(id.c_str())->getCompartmentImage());
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
