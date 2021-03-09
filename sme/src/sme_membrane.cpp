// Python.h (included by pybind11.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <pybind11/pybind11.h>

#include "model.hpp"
#include "sme_common.hpp"
#include "sme_membrane.hpp"

namespace sme {

void pybindMembrane(pybind11::module &m) {
  sme::bindList<Membrane>(m, "Membrane");
  pybind11::class_<Membrane>(m, "Membrane",
                             R"(
                             a membrane where two compartments meet
                             )")
      .def_property("name", &Membrane::getName, &Membrane::setName,
                    R"(
                    str: the name of this membrane
                    )")
      .def_readonly("reactions", &Membrane::reactions,
                    R"(
                    ReactionList: the reactions in this membrane
                    )")
      .def("__repr__",
           [](const Membrane &a) {
             return fmt::format("<sme.Membrane named '{}'>", a.getName());
           })
      .def("__str__", &Membrane::getStr);
}

Membrane::Membrane(model::Model *sbmlDocWrapper, const std::string &sId)
    : s(sbmlDocWrapper), id(sId) {
  if (auto reacs = s->getReactions().getIds(id.c_str()); !reacs.isEmpty()) {
    for (const auto &reac : reacs) {
      reactions.emplace_back(s, reac.toStdString());
    }
  }
}

std::string Membrane::getName() const {
  return s->getMembranes().getName(id.c_str()).toStdString();
}

void Membrane::setName(const std::string &name) {
  s->getMembranes().setName(id.c_str(), name.c_str());
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

//

//

//

//

//

//
