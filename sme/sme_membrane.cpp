// Python.h (included by pybind11.h) must come first:
#include <pybind11/pybind11.h>

// other headers
#include <pybind11/stl.h>

#include "model.hpp"
#include "sme_common.hpp"
#include "sme_membrane.hpp"

namespace sme {

void pybindMembrane(const pybind11::module &m) {
  pybind11::class_<sme::Membrane>(m, "Membrane",
                                  R"(
                                  a membrane where two compartments meet
                                  )")
      .def_property_readonly("name", &sme::Membrane::getName,
                             R"(
                             str: the name of this membrane
                             )")
      .def_readonly("reactions", &sme::Membrane::reactions,
                    R"(
                    list of Reaction: the reactions in this compartment
                    )")
      .def("reaction", &sme::Membrane::getReaction, pybind11::arg("name"),
           R"(
           Returns the reaction with the given name.

           Args:
               name (str): The name of the reaction

           Returns:
               Reaction: the reaction if found.

           Raises:
               InvalidArgument: if no reaction was found with this name
           )")
      .def("__repr__",
           [](const sme::Membrane &a) {
             return fmt::format("<sme.Membrane named '{}'>", a.getName());
           })
      .def("__str__", &sme::Membrane::getStr);
}

Membrane::Membrane(model::Model *sbmlDocWrapper, const std::string &sId)
    : s(sbmlDocWrapper), id(sId) {
  if (auto reacs = s->getReactions().getIds(sId.c_str()); !reacs.isEmpty()) {
    for (const auto &reac : reacs) {
      reactions.emplace_back(s, reac.toStdString());
    }
  }
}

std::string Membrane::getName() const {
  return s->getMembranes().getName(id.c_str()).toStdString();
}

const Reaction &Membrane::getReaction(const std::string &name) const {
  return findElem(reactions, name, "Reaction");
}

std::string Membrane::getStr() const {
  std::string str("<sme.Membrane>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(fmt::format("  - reactions: {}", vecToNames(reactions)));
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
