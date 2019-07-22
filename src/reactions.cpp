#include "reactions.hpp"

#include "logger.hpp"

namespace reactions {

static std::map<std::string, double> getGlobalConstants(
    sbml::SbmlDocWrapper *doc) {
  std::map<std::string, double> constants;
  const auto *model = doc->model;
  // add all *constant* species as constants
  for (unsigned k = 0; k < model->getNumSpecies(); ++k) {
    const auto *spec = model->getSpecies(k);
    if (doc->isSpeciesConstant(spec->getId())) {
      spdlog::debug(
          "reactions::getGlobalConstants :: found constant species {}",
          spec->getId());
      // todo: check if species is *also* non-spatial
      double init_conc = 0;
      // if SBML file specifies amount: convert to concentration
      if (spec->isSetInitialAmount()) {
        double amount = spec->getInitialAmount();
        double vol = model->getCompartment(spec->getCompartment())->getSize();
        init_conc = amount / vol;
        spdlog::debug(
            "reactions::getGlobalConstants :: converting amount {} to "
            "concentration {} by dividing by vol {}",
            amount, init_conc, vol);
      } else {
        init_conc = spec->getInitialConcentration();
      }
      constants[spec->getId()] = init_conc;
    }
  }
  // add any parameters (that are not replaced by an AssignmentRule)
  for (unsigned k = 0; k < model->getNumParameters(); ++k) {
    const auto *param = model->getParameter(k);
    if (model->getAssignmentRule(param->getId()) == nullptr) {
      constants[param->getId()] = param->getValue();
    }
  }
  // also get compartment volumes (the compartmentID may be used in the reaction
  // equation, and it should be replaced with the value of the "Size"
  // parameter for this compartment)
  for (unsigned int k = 0; k < model->getNumCompartments(); ++k) {
    const auto *comp = model->getCompartment(k);
    constants[comp->getId()] = comp->getSize();
  }
  return constants;
}

static std::string inlineExpr(sbml::SbmlDocWrapper *doc,
                              const std::string &expr) {
  std::string inlined;
  // inline any Function calls in expr
  inlined = doc->inlineFunctions(expr);
  // inline any Assignment Rules in expr
  inlined = doc->inlineAssignments(inlined);
  return inlined;
}

static bool addStoichCoeff(sbml::SbmlDocWrapper *doc, std::vector<double> &Mrow,
                           const libsbml::SpeciesReference *spec_ref,
                           double sign,
                           const std::vector<std::string> &speciesIDs) {
  const std::string &speciesID = spec_ref->getSpecies();
  const auto *species = doc->model->getSpecies(speciesID);
  const auto *compartment =
      doc->model->getCompartment(species->getCompartment());
  double volFactor = compartment->getSize();
  spdlog::debug(
      "reactions::addStoichCoeff :: species '{}', sign: {}, compartment "
      "volume: "
      "{}",
      speciesID, sign, volFactor);
  // if it is in the species vector, and not constant, insert into matrix M
  auto it = std::find(speciesIDs.cbegin(), speciesIDs.cend(), speciesID);
  if (it != speciesIDs.cend() && doc->isSpeciesReactive(speciesID)) {
    std::size_t speciesIndex =
        static_cast<std::size_t>(it - speciesIDs.cbegin());
    double coeff = sign * spec_ref->getStoichiometry() / volFactor;
    spdlog::debug("reactions::addStoichCoeff ::   -> stoich coeff[{}]: {}",
                  speciesIndex, coeff);
    Mrow[speciesIndex] += coeff;
    return true;
  }
  return false;
}

void Reaction::init(sbml::SbmlDocWrapper *doc_ptr,
                    const std::vector<std::string> &species,
                    const std::vector<std::string> &reactionIDs) {
  doc = doc_ptr;
  speciesIDs = species;
  spdlog::info("Reaction::Reaction :: species vector: {}", speciesIDs);
  M.clear();
  reacExpressions.clear();

  // check if any species have a RateRule
  // todo: not currently valid if raterule involves species in multiple
  // compartments
  // todo: check if should divide by volume here as well as for kinetic law
  // i.e. if the rate rule is for amount like the kinetic law
  for (std::size_t sIndex = 0; sIndex < speciesIDs.size(); ++sIndex) {
    const auto *rule = doc->model->getRateRule(speciesIDs[sIndex]);
    if (rule != nullptr) {
      std::map<std::string, double> c = getGlobalConstants(doc);
      std::string expr = inlineExpr(doc, rule->getFormula());
      std::vector<double> Mrow(speciesIDs.size(), 0);
      Mrow[sIndex] = 1.0;
      M.push_back(Mrow);
      reacExpressions.push_back(expr);
      constants.push_back(c);
      spdlog::info("Reaction::Reaction :: adding rate rule for species {}",
                   speciesIDs[sIndex]);
      spdlog::info("Reaction::Reaction ::   - expr: {}", expr);
    }
  }

  // process each reaction
  for (const auto &reacID : reactionIDs) {
    const auto *reac = doc->model->getReaction(reacID);
    bool isReaction = false;

    std::map<std::string, double> c = getGlobalConstants(doc);

    // construct row of stoichiometric coefficients for each
    // species produced and consumed by this reaction
    std::vector<double> Mrow(speciesIDs.size(), 0);
    for (unsigned k = 0; k < reac->getNumProducts(); ++k) {
      if (addStoichCoeff(doc, Mrow, reac->getProduct(k), +1.0, speciesIDs)) {
        isReaction = true;
      }
    }
    for (unsigned k = 0; k < reac->getNumReactants(); ++k) {
      if (addStoichCoeff(doc, Mrow, reac->getReactant(k), -1.0, speciesIDs)) {
        isReaction = true;
      }
    }
    if (isReaction) {
      // if matrix row is non-zero, i.e. reaction does something, then insert it
      // into the M matrix, and construct the corresponding reaction term
      M.push_back(Mrow);
      // get mathematical formula
      const auto *kin = reac->getKineticLaw();
      std::string expr = inlineExpr(doc, kin->getFormula());

      // TODO: deal with amount vs concentration issues correctly
      // if getHasOnlySubstanceUnits is true for some (all?) species
      // note: would also need to also do this in the inlining step,
      // and in the stoich matrix factors

      // get local parameters, append to global constants
      // NOTE: if a parameter is set by an assignment rule
      // it should *not* be added as a constant below:
      // (it should no longer be present in expr after inlining)
      for (unsigned k = 0; k < kin->getNumLocalParameters(); ++k) {
        const auto *param = kin->getLocalParameter(k);
        if (doc->model->getAssignmentRule(param->getId()) == nullptr) {
          c[param->getId()] = param->getValue();
        }
      }
      for (unsigned k = 0; k < kin->getNumParameters(); ++k) {
        const auto *param = kin->getParameter(k);
        if (doc->model->getAssignmentRule(param->getId()) == nullptr) {
          c[param->getId()] = param->getValue();
        }
      }
      // construct expression and add to reactions
      reacExpressions.push_back(expr);
      constants.push_back(c);
      spdlog::info("Reaction::Reaction :: adding reaction {}", reacID);
      spdlog::info("Reaction::Reaction ::   - stoichiometric matrix row: {}",
                   Mrow);
      spdlog::info("Reaction::Reaction ::   - expr: {}", expr);
    }
  }
}

Reaction::Reaction(sbml::SbmlDocWrapper *doc_ptr,
                   const std::vector<std::string> &species,
                   const std::vector<std::string> &reactionIDs) {
  init(doc_ptr, species, reactionIDs);
}

Reaction::Reaction(sbml::SbmlDocWrapper *doc_ptr, const QStringList &species,
                   const QStringList &reactionIDs) {
  std::vector<std::string> svec;
  for (const auto &s : species) {
    svec.push_back(s.toStdString());
  }
  std::vector<std::string> rvec;
  for (const auto &r : reactionIDs) {
    rvec.push_back(r.toStdString());
  }
  init(doc_ptr, svec, rvec);
}

}  // namespace reactions
