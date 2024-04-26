// Python.h (#included by nanobind.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <nanobind/nanobind.h>

#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme_common.hpp"
#include "sme_compartment.hpp"
#include <nanobind/stl/string.h>

namespace pysme {

void bindCompartment(nanobind::module_ &m) {
  bindList<Compartment>(m, "Compartment");
  nanobind::class_<Compartment>(m, "Compartment",
                                R"(
                                a compartment where species live
                                )")
      .def_prop_rw("name", &Compartment::getName, &Compartment::setName,
                   R"(
                    str: the name of this compartment
                    )")
      .def_ro("species", &Compartment::species,
              R"(
                    SpeciesList: the species in this compartment
                    )")
      .def_ro("reactions", &Compartment::reactions,
              R"(
                    ReactionList: the reactions in this compartment
                    )")
      .def_prop_ro("geometry_mask", &Compartment::geometry_mask,
                   nanobind::rv_policy::take_ownership,
                   R"(
                    numpy.ndarray: a voxel mask of the compartment geometry

                    An array of boolean values, where
                    ``geometry_mask[z][y][x] = True``
                    if the voxel at point (x,y,z) is part of this compartment

                    Examples:
                        the mask is a 3d (depth x height x width) array of bool:

                        >>> import sme
                        >>> model = sme.open_example_model()
                        >>> mask = model.compartments['Cell'].geometry_mask
                        >>> type(mask)
                        <class 'numpy.ndarray'>
                        >>> mask.dtype
                        dtype('bool')
                        >>> mask.shape
                        (1, 100, 100)

                        the first z-slice of the mask can be displayed using matplotlib:

                        >>> import matplotlib.pyplot as plt
                        >>> imgplot = plt.imshow(mask[0], interpolation='none')
                    )")
      .def("__repr__",
           [](const Compartment &a) {
             return fmt::format("<sme.Compartment named '{}'>", a.getName());
           })
      .def("__str__", &Compartment::getStr);
}

Compartment::Compartment(::sme::model::Model *sbmlDocWrapper,
                         const std::string &sId)
    : s(sbmlDocWrapper), id(sId) {
  const auto &compSpecies = s->getSpecies().getIds(id.c_str());
  species.reserve(static_cast<std::size_t>(compSpecies.size()));
  for (const auto &spec : compSpecies) {
    species.emplace_back(s, spec.toStdString());
  }
  if (auto reacs = s->getReactions().getIds(id.c_str()); !reacs.isEmpty()) {
    for (const auto &reac : reacs) {
      reactions.emplace_back(s, reac.toStdString());
    }
  }
}

std::string Compartment::getName() const {
  return s->getCompartments().getName(id.c_str()).toStdString();
}

void Compartment::setName(const std::string &name) {
  s->getCompartments().setName(id.c_str(), name.c_str());
}

nanobind::ndarray<nanobind::numpy, bool> Compartment::geometry_mask() const {
  return toPyImageMask(
      s->getCompartments().getCompartment(id.c_str())->getCompartmentImages());
}

std::string Compartment::getStr() const {
  std::string str("<sme.Compartment>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(fmt::format("  - species: {}", vecToNames(species)));
  return str;
}

} // namespace pysme
