#include "sme_reactionparameter.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "sbml.hpp"
#include "sme_common.hpp"

namespace sme {

void pybindReactionParameter(const pybind11::module& m) {
  pybind11::class_<sme::ReactionParameter>(m, "ReactionParameter")
      .def_property("name", &sme::ReactionParameter::getName,
                    &sme::ReactionParameter::setName,
                    "The name of this reaction parameter")
      .def_property("value", &sme::ReactionParameter::getValue,
                    &sme::ReactionParameter::setValue,
                    "The value of this reaction parameter")
      .def("__repr__",
           [](const sme::ReactionParameter& a) {
             return fmt::format("<sme.ReactionParameter named '{}'>",
                                a.getName());
           })
      .def("__str__", &sme::ReactionParameter::getStr);
}

ReactionParameter::ReactionParameter(sbml::SbmlDocWrapper* sbmlDocWrapper,
                                     const std::string& reactionId,
                                     const std::string& parameterId)
    : s(sbmlDocWrapper), reacId(reactionId), paramId(parameterId) {}

void ReactionParameter::setName(const std::string& name) {
  auto r = s->getReaction(reacId.c_str());
  if (auto iter =
          std::find_if(r.constants.begin(), r.constants.end(),
                       [&id = paramId](const auto& c) { return c.id == id; });
      iter != r.constants.end()) {
    iter->name = name;
    s->setReaction(r);
  }
}

std::string ReactionParameter::getName() const {
  auto r = s->getReaction(reacId.c_str());
  if (auto iter =
          std::find_if(r.constants.cbegin(), r.constants.cend(),
                       [&id = paramId](const auto& c) { return c.id == id; });
      iter != r.constants.cend()) {
    return iter->name;
  }
  return {};
}

void ReactionParameter::setValue(double value) {
  auto r = s->getReaction(reacId.c_str());
  if (auto iter =
          std::find_if(r.constants.begin(), r.constants.end(),
                       [&id = paramId](const auto& c) { return c.id == id; });
      iter != r.constants.end()) {
    iter->value = value;
    s->setReaction(r);
  }
}

double ReactionParameter::getValue() const {
  auto r = s->getReaction(reacId.c_str());
  if (auto iter =
          std::find_if(r.constants.cbegin(), r.constants.cend(),
                       [&id = paramId](const auto& c) { return c.id == id; });
      iter != r.constants.cend()) {
    return iter->value;
  }
  return {};
}

std::string ReactionParameter::getStr() const {
  std::string str("<sme.ReactionParameter>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(fmt::format("  - value: '{}'\n", getValue()));
  return str;
}

}  // namespace sme
