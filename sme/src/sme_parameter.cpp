// Python.h (#included by nanobind.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <nanobind/nanobind.h>

#include "sme/model.hpp"
#include "sme_common.hpp"
#include "sme_parameter.hpp"
#include <nanobind/stl/string.h>

namespace pysme {

void bindParameter(nanobind::module_ &m) {
  bindList<Parameter>(m, "Parameter");
  nanobind::class_<Parameter>(m, "Parameter",
                              R"(
                              a parameter of the model
                              )")
      .def_prop_rw("name", &Parameter::getName, &Parameter::setName,
                   R"(
                    str: the name of this parameter
                    )")
      .def_prop_rw("value", &Parameter::getValue, &Parameter::setValue,
                   R"(
                    str: the mathematical expression for this reaction parameter
                    )")
      .def("__repr__",
           [](const Parameter &a) {
             return fmt::format("<sme.Parameter named '{}'>", a.getName());
           })
      .def("__str__", &Parameter::getStr);
}
Parameter::Parameter(::sme::model::Model *sbmlDocWrapper,
                     const std::string &sId)
    : s(sbmlDocWrapper), id(sId) {}

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

} // namespace pysme
