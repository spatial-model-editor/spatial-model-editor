// Python.h (included by pybind11.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <pybind11/pybind11.h>

#include "model.hpp"
#include "sme_common.hpp"
#include "sme_reaction.hpp"
#include <pybind11/stl_bind.h>

namespace sme {

void pybindReaction(pybind11::module &m) {
  pybind11::bind_vector<std::vector<Reaction>>(m, "ReactionList",
                                               R"(
                                               a list of reactions
                                               )");

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
      .def("parameter", &Reaction::getParameter, pybind11::arg("name"),
           R"(
           Returns the reaction parameter with the given name.

           Args:
               name (str): The name of the reaction parameter

           Returns:
               ReactionParameter: the reaction parameter if found.

           Raises:
               InvalidArgument: if no reaction parameter was found with this name
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

const ReactionParameter &Reaction::getParameter(const std::string &name) const {
  return findElem(parameters, name, "ReactionParameter");
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
