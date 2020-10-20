// Python.h (included by pybind11.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <pybind11/pybind11.h>

#include "model.hpp"
#include "sme_common.hpp"
#include "sme_species.hpp"

namespace sme {

void pybindSpecies(pybind11::module &m) {
  sme::bindList<Species>(m, "Species");
  pybind11::class_<Species>(m, "Species",
                            R"(
                            a species that lives in a compartment
                            )")
      .def_property("name", &Species::getName, &Species::setName,
                    R"(
                    str: the name of this species
                    )")
      .def_property("diffusion_constant", &Species::getDiffusionConstant,
                    &Species::setDiffusionConstant,
                    R"(
                    float: the diffusion constant of this species
                    )")
      .def("__repr__",
           [](const Species &a) {
             return fmt::format("<sme.Species named '{}'>", a.getName());
           })
      .def("__str__", &Species::getStr);
}

Species::Species(model::Model *sbmlDocWrapper, const std::string &sId)
    : s(sbmlDocWrapper), id(sId) {}

void Species::setName(const std::string &name) {
  s->getSpecies().setName(id.c_str(), name.c_str());
}

std::string Species::getName() const {
  return s->getSpecies().getName(id.c_str()).toStdString();
}

void Species::setDiffusionConstant(double diffusionConstant) {
  s->getSpecies().setDiffusionConstant(id.c_str(), diffusionConstant);
}

double Species::getDiffusionConstant() const {
  return s->getSpecies().getDiffusionConstant(id.c_str());
}

std::string Species::getStr() const {
  std::string str("<sme.Species>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(
      fmt::format("  - diffusion_constant: {}\n", getDiffusionConstant()));
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
