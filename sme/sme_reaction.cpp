#include "sme_reaction.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "model.hpp"
#include "sme_common.hpp"

namespace sme {

void pybindReaction(const pybind11::module &m) {
  pybind11::class_<sme::Reaction>(m, "Reaction")
      .def_property("name", &sme::Reaction::getName, &sme::Reaction::setName,
                    "The name of this reaction")
      .def_readonly("parameters", &sme::Reaction::parameters,
                    "The parameters in this reaction")
      .def("parameter", &sme::Reaction::getParameter, pybind11::arg("name"))
      .def("__repr__",
           [](const sme::Reaction &a) {
             return fmt::format("<sme.Reaction named '{}'>", a.getName());
           })
      .def("__str__", &sme::Reaction::getStr);
}

Reaction::Reaction(model::Model *sbmlDocWrapper, const std::string &sId)
    : s(sbmlDocWrapper), id(sId) {
  const auto &paramIds = s->getReactions().getParameterIds(id.c_str());
  parameters.reserve(static_cast<std::size_t>(paramIds.size()));
  for (const auto &paramId : paramIds) {
    parameters.emplace_back(s, id, paramId.toStdString());
  }
}

std::string Reaction::getName() const {
  return s->getReactions().getName(id.c_str()).toStdString();
}

void Reaction::setName(const std::string &name) {
  s->getReactions().setName(id.c_str(), name.c_str());
}

const ReactionParameter &Reaction::getParameter(const std::string &name) const {
  return findElem(parameters, name, "ReactionParameter");
}

std::string Reaction::getStr() const {
  std::string str("<sme.Reaction>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  return str;
}

} // namespace sme
