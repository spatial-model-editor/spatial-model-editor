#include "pde.hpp"

#include <optional>

#include "logger.hpp"
#include "sbml.hpp"
#include "symbolic.hpp"
#include "utils.hpp"

namespace pde {

PDE::PDE(const sbml::SbmlDocWrapper *doc_ptr,
         const std::vector<std::string> &speciesIDs,
         const std::vector<std::string> &reactionIDs,
         const std::vector<std::string> &relabelledSpeciesIDs,
         const std::vector<std::string> &reactionScaleFactors)
    : species(speciesIDs) {
  if (!relabelledSpeciesIDs.empty() &&
      relabelledSpeciesIDs.size() != speciesIDs.size()) {
    SPDLOG_WARN(
        "Ignoring relabelledSpecies:"
        "size {} does not match number of species {}",
        relabelledSpeciesIDs.size(), speciesIDs.size());
  }
  if (!reactionScaleFactors.empty() &&
      reactionScaleFactors.size() != reactionIDs.size()) {
    SPDLOG_WARN(
        "Ignoring reactionScaleFactors:"
        "size {} does not match number of reactions {}",
        reactionScaleFactors.size(), reactionIDs.size());
  }
  // construct reaction expressions and stoich matrix
  Reaction reactions(doc_ptr, speciesIDs, reactionIDs);

  // construct symbolic expressions: one rhs + Jacobian for each species
  rhs.clear();
  jacobian.clear();
  for (std::size_t i = 0; i < speciesIDs.size(); ++i) {
    jacobian.emplace_back();
    QString r("0.0");
    for (std::size_t j = 0; j < reactions.size(); ++j) {
      // get reaction term
      QString expr =
          QString("%1*(%2) ")
              .arg(QString::number(reactions.getMatrixElement(j, i), 'g', 18),
                   reactions.getExpression(j).c_str());
      // rescale by supplied reactionScaleFactor
      QString scaleFactor("1");
      if (reactionScaleFactors.size() == reactionIDs.size()) {
        scaleFactor = reactionScaleFactors[j].c_str();
      }
      expr = QString("((%1)/%2) ").arg(expr, scaleFactor);
      SPDLOG_DEBUG("Species {} Reaction {} = {}", speciesIDs.at(i), j,
                   expr.toStdString());
      // parse and inline constants
      symbolic::Symbolic sym(expr.toStdString(), reactions.getSpeciesIDs(),
                             reactions.getConstants(j));
      // add term to rhs
      r.append(QString(" + (%1)").arg(sym.simplify().c_str()));
    }
    // reparse full rhs to simplify
    SPDLOG_DEBUG("Species {} Reparsing all reaction terms", speciesIDs.at(i));
    // parse expression with symengine to simplify
    symbolic::Symbolic sym(r.toStdString(), speciesIDs);
    // if provided, relabel species with relabelledSpeciesIDs
    auto *outputSpecies = &speciesIDs;
    if (relabelledSpeciesIDs.size() == speciesIDs.size()) {
      sym.relabel(relabelledSpeciesIDs);
      outputSpecies = &relabelledSpeciesIDs;
    }
    for (const auto &s : *outputSpecies) {
      jacobian.back().push_back(sym.diff(s));
    }
    rhs.push_back(sym.simplify());
  }
}

const std::vector<std::string> &PDE::getRHS() const { return rhs; }
const std::vector<std::vector<std::string>> &PDE::getJacobian() const {
  return jacobian;
}

// return index of species if in the species vector, and reactive
static std::optional<std::size_t> getSpeciesIndex(
    const sbml::SbmlDocWrapper *doc, const std::string &speciesID,
    const std::vector<std::string> &speciesIDs) {
  if (auto it = std::find(speciesIDs.cbegin(), speciesIDs.cend(), speciesID);
      it != speciesIDs.cend() && doc->isSpeciesReactive(speciesID)) {
    return static_cast<std::size_t>(it - speciesIDs.cbegin());
  }
  return {};
}

std::vector<double> Reaction::getStoichMatrixRow(
    const sbml::SbmlDocWrapper *doc, const sbml::Reac &reac) const {
  std::vector<double> matrixRow(speciesIDs.size(), 0);
  bool isReaction = false;
  for (const auto &[speciesID, speciesName, stoichCoeff] : reac.species) {
    SPDLOG_DEBUG("product '{}'", speciesID);
    auto speciesIndex = getSpeciesIndex(doc, speciesID, speciesIDs);
    if (speciesIndex) {
      SPDLOG_DEBUG("  -> stoich coeff[{}]: {:16.16e}", speciesIndex.value(),
                   stoichCoeff);
      matrixRow[speciesIndex.value()] += stoichCoeff;
      isReaction = true;
    }
  }
  if (isReaction) {
    return matrixRow;
  }
  return {};
}

std::size_t Reaction::size() const { return expressions.size(); }

const std::string &Reaction::getExpression(std::size_t reactionIndex) const {
  return expressions.at(reactionIndex);
}

double Reaction::getMatrixElement(std::size_t speciesIndex,
                                  std::size_t reactionIndex) const {
  return M.at(speciesIndex).at(reactionIndex);
}

const std::vector<std::string> &Reaction::getSpeciesIDs() const {
  return speciesIDs;
}

const std::map<std::string, double> &Reaction::getConstants(
    std::size_t reactionIndex) const {
  return constants.at(reactionIndex);
}

Reaction::Reaction(const sbml::SbmlDocWrapper *doc,
                   const std::vector<std::string> &species,
                   const std::vector<std::string> &reactionIDs)
    : speciesIDs(species) {
  // RateRule is f(...) for a species s: ds/dt = f(...)
  // Species which have a raterule cannot be created or destroyed in any kinetic
  // reactions
  // todo: not currently valid if raterule involves species in multiple
  // compartments
  for (std::size_t sIndex = 0; sIndex < speciesIDs.size(); ++sIndex) {
    auto ruleExpr = doc->getRateRule(speciesIDs[sIndex]);
    if (!ruleExpr.empty()) {
      std::map<std::string, double> c = doc->getGlobalConstants();
      std::vector<double> Mrow(speciesIDs.size(), 0);
      Mrow[sIndex] = 1.0;
      M.push_back(Mrow);
      expressions.push_back(ruleExpr);
      constants.push_back(c);
      SPDLOG_INFO("adding rate rule for species {}", speciesIDs[sIndex]);
      SPDLOG_INFO("  - expr: {}", ruleExpr);
    }
  }

  // process each reaction
  for (const auto &reacID : reactionIDs) {
    auto reac = doc->getReaction(reacID.c_str());

    // construct row of stoichiometric coefficients for each
    // species produced and consumed by this reaction
    auto row = getStoichMatrixRow(doc, reac);
    if (!row.empty()) {
      // if matrix row is non-zero, i.e. reaction does something, then insert
      // it into the M matrix, and construct the corresponding reaction term
      M.push_back(row);

      // get local parameters, append to global constants
      std::map<std::string, double> c = doc->getGlobalConstants();
      for (const auto &[id, name, value] : reac.constants) {
        c[id] = value;
      }
      // construct expression and add to reactions
      auto inlinedExpr = doc->inlineExpr(reac.expression);
      expressions.push_back(inlinedExpr);
      constants.push_back(c);
      SPDLOG_INFO("adding reaction {}", reacID);
      SPDLOG_INFO("  - stoichiometric matrix row: {}",
                  utils::vectorToString(row));
      SPDLOG_INFO("  - expr: {}", inlinedExpr);
    }
  }
}

}  // namespace pde
