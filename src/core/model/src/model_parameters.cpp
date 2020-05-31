#include "model_parameters.hpp"

#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

#include <QString>
#include <memory>

#include "logger.hpp"
#include "model_species.hpp"
#include "sbml_utils.hpp"
#include "xml_annotation.hpp"

namespace model {

ModelParameters::ModelParameters() = default;

ModelParameters::ModelParameters(libsbml::Model *model) : sbmlModel{model} {}

std::vector<IdNameValue> ModelParameters::getGlobalConstants() const {
  std::vector<IdNameValue> constants;
  // add all *constant* species as constants
  for (unsigned k = 0; k < sbmlModel->getNumSpecies(); ++k) {
    const auto *spec = sbmlModel->getSpecies(k);
    if (getIsSpeciesConstant(spec)) {
      SPDLOG_TRACE("found constant species {}", spec->getId());
      double init_conc = spec->getInitialConcentration();
      constants.push_back({spec->getId(), spec->getName(), init_conc});
      SPDLOG_TRACE("parameter {} = {}", spec->getId(), init_conc);
    }
  }
  // add any parameters (that are not replaced by an AssignmentRule)
  for (unsigned k = 0; k < sbmlModel->getNumParameters(); ++k) {
    const auto *param = sbmlModel->getParameter(k);
    if (sbmlModel->getAssignmentRule(param->getId()) == nullptr) {
      SPDLOG_TRACE("parameter {} = {}", param->getId(), param->getValue());
      if (!(param->getId() == "x" || param->getId() == "y")) {
        // remove x and y if present, as these are not really parameters
        // (we want them to remain as variables to be parsed by symbolic
        // parser) todo: check if this can be done in a better way
        constants.push_back(
            {param->getId(), param->getName(), param->getValue()});
      }
    }
  }
  // also get compartment volumes (the compartmentID may be used in the
  // reaction equation, and it should be replaced with the value of the "Size"
  // parameter for this compartment)
  for (unsigned int k = 0; k < sbmlModel->getNumCompartments(); ++k) {
    const auto *comp = sbmlModel->getCompartment(k);
    SPDLOG_TRACE("parameter {} = {}", comp->getId(), comp->getSize());
    constants.push_back({comp->getId(), comp->getName(), comp->getSize()});
  }
  return constants;
}

std::vector<IdNameExpr> ModelParameters::getNonConstantParameters() const {
  std::vector<IdNameExpr> rules;
  for (unsigned k = 0; k < sbmlModel->getNumRules(); ++k) {
    if (const auto *rule = sbmlModel->getRule(k); rule->isAssignment()) {
      const auto &sId = rule->getVariable();
      rules.push_back(
          {sId, sbmlModel->getParameter(sId)->getName(), rule->getFormula()});
    }
  }
  return rules;
}
}  // namespace sbml
