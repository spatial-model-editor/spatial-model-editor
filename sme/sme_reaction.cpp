#include "sme_reaction.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "sbml.hpp"
#include "sme_common.hpp"

namespace sme {

void pybindReaction(const pybind11::module& m) {
  pybind11::class_<sme::Reaction>(m, "Reaction")
      .def_property("name", &sme::Reaction::getName, &sme::Reaction::setName,
                    "The name of this reaction")
      .def("__repr__",
           [](const sme::Reaction& a) {
             return fmt::format("<sme.Reaction named '{}'>", a.getName());
           })
      .def("__str__", &sme::Reaction::getStr);
}

Reaction::Reaction(sbml::SbmlDocWrapper* sbmlDocWrapper,
                   const std::string& ReactionId)
    : s(sbmlDocWrapper), id(ReactionId) {}

const std::string& Reaction::getId() const { return id; }

void Reaction::setName(const std::string& name) {
  auto r = s->getReaction(id.c_str());
  r.name = name;
  s->setReaction(r);
}

std::string Reaction::getName() const {
  return s->getReactionName(id.c_str()).toStdString();
}

std::string Reaction::getStr() const {
  std::string str("<sme.Reaction>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  return str;
}

}  // namespace sme
