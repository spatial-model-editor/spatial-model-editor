#include <pybind11/pybind11.h>

#include "sme_compartment.hpp"
#include "sme_model.hpp"
#include "sme_module.hpp"
#include "sme_species.hpp"
#include "version.hpp"

PYBIND11_MODULE(sme, m) {
  sme::pybindModule(m);
  sme::pybindModel(m);
  sme::pybindCompartment(m);
  sme::pybindSpecies(m);
}
