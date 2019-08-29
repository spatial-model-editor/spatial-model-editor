#include "reactions.hpp"

#include "logger.hpp"

namespace reactions {

static bool addStoichCoeff(const sbml::SbmlDocWrapper *doc,
                           std::vector<double> &Mrow,
                           const libsbml::SpeciesReference *spec_ref,
                           double sign,
                           const std::vector<std::string> &speciesIDs) {
  const std::string &speciesID = spec_ref->getSpecies();
  double volFactor = doc->getSpeciesCompartmentSize(speciesID.c_str());
  SPDLOG_DEBUG("species '{}', sign: {}, compartment volume: {}", speciesID,
               sign, volFactor);
  // if it is in the species vector, and not constant, insert into matrix M
  auto it = std::find(speciesIDs.cbegin(), speciesIDs.cend(), speciesID);
  if (it != speciesIDs.cend() && doc->isSpeciesReactive(speciesID)) {
    std::size_t speciesIndex =
        static_cast<std::size_t>(it - speciesIDs.cbegin());
    double coeff = sign * spec_ref->getStoichiometry() / volFactor;
    SPDLOG_DEBUG("  -> stoich coeff[{}]: {}", speciesIndex, coeff);
    Mrow[speciesIndex] += coeff;
    return true;
  }
  return false;
}

void Reaction::init(const sbml::SbmlDocWrapper *doc_ptr,
                    const std::vector<std::string> &species,
                    const std::vector<std::string> &reactionIDs) {
  doc = doc_ptr;
  speciesIDs = species;
  SPDLOG_INFO("species vector: {}", speciesIDs);
  M.clear();
  reacExpressions.clear();

  // check if any species have a RateRule
  // todo: not currently valid if raterule involves species in multiple
  // compartments
  // todo: check if should divide by volume here as well as for kinetic law
  // i.e. if the rate rule is for amount like the kinetic law
  for (std::size_t sIndex = 0; sIndex < speciesIDs.size(); ++sIndex) {
    const auto *rule = doc->getRateRule(speciesIDs[sIndex]);
    if (rule != nullptr) {
      std::map<std::string, double> c = doc->getGlobalConstants();
      std::string expr = doc->inlineExpr(rule->getFormula());
      std::vector<double> Mrow(speciesIDs.size(), 0);
      Mrow[sIndex] = 1.0;
      M.push_back(Mrow);
      reacExpressions.push_back(expr);
      constants.push_back(c);
      SPDLOG_INFO("adding rate rule for species {}", speciesIDs[sIndex]);
      SPDLOG_INFO("  - expr: {}", expr);
    }
  }

  // process each reaction
  for (const auto &reacID : reactionIDs) {
    const auto *reac = doc->getReaction(reacID.c_str());
    bool isReaction = false;

    std::map<std::string, double> c = doc->getGlobalConstants();

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
      std::string expr = doc->inlineExpr(kin->getFormula());

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
        if (doc->getAssignmentRule(param->getId()) == nullptr) {
          c[param->getId()] = param->getValue();
        }
      }
      for (unsigned k = 0; k < kin->getNumParameters(); ++k) {
        const auto *param = kin->getParameter(k);
        if (doc->getAssignmentRule(param->getId()) == nullptr) {
          c[param->getId()] = param->getValue();
        }
      }
      // construct expression and add to reactions
      reacExpressions.push_back(expr);
      constants.push_back(c);
      SPDLOG_INFO("adding reaction {}", reacID);
      SPDLOG_INFO("  - stoichiometric matrix row: {}", Mrow);
      SPDLOG_INFO("  - expr: {}", expr);
    }
  }
}

Reaction::Reaction(const sbml::SbmlDocWrapper *doc_ptr,
                   const std::vector<std::string> &species,
                   const std::vector<std::string> &reactionIDs) {
  init(doc_ptr, species, reactionIDs);
}

Reaction::Reaction(const sbml::SbmlDocWrapper *doc_ptr,
                   const QStringList &species, const QStringList &reactionIDs) {
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
