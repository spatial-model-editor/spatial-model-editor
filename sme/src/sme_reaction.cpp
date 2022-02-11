// Python.h (included by pybind11.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <pybind11/pybind11.h>

#include "sme/model.hpp"
#include "sme_common.hpp"
#include "sme_reaction.hpp"

namespace sme {

void pybindReaction(pybind11::module &m) {
  sme::bindList<Reaction>(m, "Reaction");
  pybind11::class_<Reaction>(m, "Reaction",
                             R"(
                             a reaction between species
                             )")
      .def_property("name", &Reaction::getName, &Reaction::setName,
                    R"(
                    str: the name of this reaction
                    )")
      .def_readonly("parameters", &Reaction::parameters,
                    R"(
                    ReactionParameterList: the parameters of this reaction
                    )")
      .def("__repr__",
           [](const Reaction &a) {
             return fmt::format("<sme.Reaction named '{}'>", a.getName());
           })
      .def("__str__", &Reaction::getStr);
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

std::string Reaction::getStr() const {
  std::string str("<sme.Reaction>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
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
