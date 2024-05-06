// Python.h (#included by nanobind.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <nanobind/nanobind.h>

#include "sme_common.hpp"
#include "sme_compartment.hpp"
#include "sme_membrane.hpp"
#include "sme_model.hpp"
#include "sme_module.hpp"
#include "sme_parameter.hpp"
#include "sme_reaction.hpp"
#include "sme_reactionparameter.hpp"
#include "sme_simulationresult.hpp"
#include "sme_species.hpp"

NB_MODULE(sme, m) {
  Q_INIT_RESOURCE(resources);
  pysme::bindModule(m);
  pysme::bindModel(m);
  pysme::bindCompartment(m);
  pysme::bindMembrane(m);
  pysme::bindSpecies(m);
  pysme::bindParameter(m);
  pysme::bindReaction(m);
  pysme::bindReactionParameter(m);
  pysme::bindSimulationResult(m);
}
