// Python.h (included by pybind11.h) must come first:
#include <pybind11/pybind11.h>

// other headers
#include "sme_compartment.hpp"
#include "sme_membrane.hpp"
#include "sme_model.hpp"
#include "sme_module.hpp"
#include "sme_simulationresult.hpp"
#include "sme_species.hpp"
#include "version.hpp"

PYBIND11_MODULE(sme, m) {
  Q_INIT_RESOURCE(resources);
  sme::pybindModule(m);
  sme::pybindModel(m);
  sme::pybindCompartment(m);
  sme::pybindMembrane(m);
  sme::pybindSpecies(m);
  sme::pybindParameter(m);
  sme::pybindReaction(m);
  sme::pybindReactionParameter(m);
  sme::pybindSimulationResult(m);
}
