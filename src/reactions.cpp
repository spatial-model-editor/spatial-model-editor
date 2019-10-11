#include "reactions.hpp"

#include "logger.hpp"
#include "sbml.hpp"
#include "utils.hpp"

namespace reactions {

static bool addStoichCoeff(
    const sbml::SbmlDocWrapper *doc, std::vector<double> &Mrow,
    const std::vector<std::pair<std::string, double>> &reacSpecies, double sign,
    const std::vector<std::string> &speciesIDs) {
  bool isReaction = false;
  for (const auto &reacSpec : reacSpecies) {
    const std::string &speciesID = reacSpec.first;
    double volFactor = doc->getSpeciesCompartmentSize(speciesID.c_str());
    SPDLOG_DEBUG("species '{}', sign: {}, compartment volume: {}", speciesID,
                 sign, volFactor);
    // if it is in the species vector, and not constant, insert into matrix M
    auto it = std::find(speciesIDs.cbegin(), speciesIDs.cend(), speciesID);
    if (it != speciesIDs.cend() && doc->isSpeciesReactive(speciesID)) {
      std::size_t speciesIndex =
          static_cast<std::size_t>(it - speciesIDs.cbegin());
      double coeff = sign * reacSpec.second / volFactor;
      SPDLOG_DEBUG("  -> stoich coeff[{}]: {:16.16e}", speciesIndex, coeff);
      Mrow[speciesIndex] += coeff;
      isReaction = true;
    }
  }
  return isReaction;
}

void Reaction::init(const sbml::SbmlDocWrapper *doc,
                    const std::vector<std::string> &species,
                    const std::vector<std::string> &reactionIDs) {
  speciesIDs = species;
  M.clear();
  reacExpressions.clear();
  constants.clear();

  // check if any species have a RateRule
  // todo: not currently valid if raterule involves species in multiple
  // compartments
  // todo: check if should divide by volume here as well as for kinetic law
  // i.e. if the rate rule is for amount like the kinetic law
  for (std::size_t sIndex = 0; sIndex < speciesIDs.size(); ++sIndex) {
    auto ruleExpr = doc->getRateRule(speciesIDs[sIndex]);
    if (!ruleExpr.empty()) {
      std::map<std::string, double> c = doc->getGlobalConstants();
      std::vector<double> Mrow(speciesIDs.size(), 0);
      Mrow[sIndex] = 1.0;
      M.push_back(Mrow);
      reacExpressions.push_back(ruleExpr);
      constants.push_back(c);
      SPDLOG_INFO("adding rate rule for species {}", speciesIDs[sIndex]);
      SPDLOG_INFO("  - expr: {}", ruleExpr);
    }
  }

  // process each reaction
  for (const auto &reacID : reactionIDs) {
    auto reac = doc->getReaction(reacID.c_str());
    std::map<std::string, double> c = doc->getGlobalConstants();

    // construct row of stoichiometric coefficients for each
    // species produced and consumed by this reaction
    std::vector<double> Mrow(speciesIDs.size(), 0);
    bool hasProducts =
        addStoichCoeff(doc, Mrow, reac.products, +1.0, speciesIDs);
    bool hasReactants =
        addStoichCoeff(doc, Mrow, reac.reactants, -1.0, speciesIDs);
    if (hasProducts || hasReactants) {
      // if matrix row is non-zero, i.e. reaction does something, then insert it
      // into the M matrix, and construct the corresponding reaction term
      M.push_back(Mrow);

      // TODO: deal with amount vs concentration issues correctly
      // if getHasOnlySubstanceUnits is true for some (all?) species
      // note: would also need to also do this in the inlining step,
      // and in the stoich matrix factors

      // get local parameters, append to global constants
      for (const auto &constant : reac.constants) {
        c[constant.first] = constant.second;
      }
      // construct expression and add to reactions
      reacExpressions.push_back(reac.expression);
      constants.push_back(c);
      SPDLOG_INFO("adding reaction {}", reacID);
      SPDLOG_INFO("  - stoichiometric matrix row: {}",
                  utils::vectorToString(Mrow));
      SPDLOG_INFO("  - expr: {}", reac.expression);
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
  init(doc_ptr, utils::toStdString(species), utils::toStdString(reactionIDs));
}

}  // namespace reactions
