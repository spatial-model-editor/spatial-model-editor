// Python.h (included by pybind11.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <pybind11/pybind11.h>

#include "sme/model.hpp"
#include "sme_common.hpp"
#include "sme_reactionparameter.hpp"

namespace sme {

void pybindReactionParameter(pybind11::module &m) {
  sme::bindList<ReactionParameter>(m, "ReactionParameter");
  pybind11::class_<ReactionParameter>(m, "ReactionParameter",
                                      R"(
                                      a parameter of a reaction
                                      )")
      .def_property("name", &ReactionParameter::getName,
                    &ReactionParameter::setName,
                    R"(
                    str: the name of this reaction parameter
                    )")
      .def_property("value", &ReactionParameter::getValue,
                    &ReactionParameter::setValue,
                    R"(
                    float: the value of this reaction parameter
                    )")
      .def("__repr__",
           [](const ReactionParameter &a) {
             return fmt::format("<sme.ReactionParameter named '{}'>",
                                a.getName());
           })
      .def("__str__", &ReactionParameter::getStr);
}

ReactionParameter::ReactionParameter(model::Model *sbmlDocWrapper,
                                     const std::string &reactionId,
                                     const std::string &parameterId)
    : s(sbmlDocWrapper), reacId(reactionId), paramId(parameterId) {}

std::string ReactionParameter::getName() const {
  return s->getReactions()
      .getParameterName(reacId.c_str(), paramId.c_str())
      .toStdString();
}

void ReactionParameter::setName(const std::string &name) {
  s->getReactions().setParameterName(reacId.c_str(), paramId.c_str(),
                                     name.c_str());
}

double ReactionParameter::getValue() const {
  return s->getReactions().getParameterValue(reacId.c_str(), paramId.c_str());
}

void ReactionParameter::setValue(double value) {
  s->getReactions().setParameterValue(reacId.c_str(), paramId.c_str(), value);
}

std::string ReactionParameter::getStr() const {
  std::string str("<sme.ReactionParameter>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(fmt::format("  - value: '{}'\n", getValue()));
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

//

//
