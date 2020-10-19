// Python.h (included by pybind11.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <pybind11/pybind11.h>

#include "sme_common.hpp"
#include "sme_compartment.hpp"
#include "sme_exception.hpp"
#include "sme_membrane.hpp"
#include "sme_model.hpp"
#include "sme_module.hpp"
#include "sme_parameter.hpp"
#include "sme_reaction.hpp"
#include "sme_reactionparameter.hpp"
#include "sme_simulationresult.hpp"
#include "sme_species.hpp"

PYBIND11_MODULE(sme, m) {
  Q_INIT_RESOURCE(resources);
  sme::pybindModule(m);
  sme::pybindException(m);
  sme::pybindModel(m);
  sme::pybindCompartment(m);
  sme::pybindMembrane(m);
  sme::pybindSpecies(m);
  sme::pybindParameter(m);
  sme::pybindReaction(m);
  sme::pybindReactionParameter(m);
  sme::pybindSimulationResult(m);
}
