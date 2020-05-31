#include "sme_reactionparameter.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "model.hpp"
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

ReactionParameter::ReactionParameter(model::Model* sbmlDocWrapper,
                                     const std::string& reactionId,
                                     const std::string& parameterId)
    : s(sbmlDocWrapper), reacId(reactionId), paramId(parameterId) {}

void ReactionParameter::setName(const std::string& name) {
  s->getReactions().setParameterName(reacId.c_str(), paramId.c_str(),
                                     name.c_str());
}

std::string ReactionParameter::getName() const {
  return s->getReactions()
      .getParameterName(reacId.c_str(), paramId.c_str())
      .toStdString();
}

void ReactionParameter::setValue(double value) {
  s->getReactions().setParameterValue(reacId.c_str(), paramId.c_str(), value);
}

double ReactionParameter::getValue() const {
  return s->getReactions().getParameterValue(reacId.c_str(), paramId.c_str());
}

std::string ReactionParameter::getStr() const {
  std::string str("<sme.ReactionParameter>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(fmt::format("  - value: '{}'\n", getValue()));
  return str;
}

}  // namespace sme
