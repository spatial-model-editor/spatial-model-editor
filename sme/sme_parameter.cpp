// Python.h (included by pybind11.h) must come first:
#include <pybind11/pybind11.h>

// other headers
#include <pybind11/stl.h>

#include <utility>

#include "model.hpp"
#include "sme_common.hpp"
#include "sme_parameter.hpp"

namespace sme {

void pybindParameter(const pybind11::module &m) {
  pybind11::class_<sme::Parameter>(m, "Parameter",
                                   R"(
                                   a parameter of the model
                                   )")
      .def_property("name", &sme::Parameter::getName, &sme::Parameter::setName,
                    R"(
                    str: the name of this parameter
                    )")
      .def_property("value", &sme::Parameter::getValue,
                    &sme::Parameter::setValue,
                    R"(
                    str: the mathematical expression for this reaction parameter
                    )")
      .def("__repr__",
           [](const sme::Parameter &a) {
             return fmt::format("<sme.Parameter named '{}'>", a.getName());
           })
      .def("__str__", &sme::Parameter::getStr);
}

Parameter::Parameter(model::Model *sbmlDocWrapper, std::string sId)
    : s(sbmlDocWrapper), id(std::move(sId)) {}

void Parameter::setName(const std::string &name) {
  s->getParameters().setName(id.c_str(), name.c_str());
}

std::string Parameter::getName() const {
  return s->getParameters().getName(id.c_str()).toStdString();
}

void Parameter::setValue(const std::string &expr) {
  s->getParameters().setExpression(id.c_str(), expr.c_str());
}

std::string Parameter::getValue() const {
  return s->getParameters().getExpression(id.c_str()).toStdString();
}

std::string Parameter::getStr() const {
  std::string str("<sme.Parameter>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(fmt::format("  - expression: '{}'\n", getValue()));
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
