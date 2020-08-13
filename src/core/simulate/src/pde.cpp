#include "pde.hpp"
#include "logger.hpp"
#include "model.hpp"
#include "model_parameters.hpp"
#include "model_reactions.hpp"
#include "model_species.hpp"
#include "symbolic.hpp"
#include "utils.hpp"
#include <QList>
#include <QString>
#include <QStringList>
#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

namespace simulate {

PDE::PDE(const model::Model *doc_ptr,
         const std::vector<std::string> &speciesIDs,
         const std::vector<std::string> &reactionIDs,
         const std::vector<std::string> &relabelledSpeciesIDs,
         const std::vector<std::string> &reactionScaleFactors)
    : species(speciesIDs) {
  if (!relabelledSpeciesIDs.empty() &&
      relabelledSpeciesIDs.size() != speciesIDs.size()) {
    SPDLOG_WARN("Ignoring relabelledSpecies:"
                "size {} does not match number of species {}",
                relabelledSpeciesIDs.size(), speciesIDs.size());
  }
  if (!reactionScaleFactors.empty() &&
      reactionScaleFactors.size() != reactionIDs.size()) {
    SPDLOG_WARN("Ignoring reactionScaleFactors:"
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
              .arg(utils::dblToQStr(reactions.getMatrixElement(j, i)),
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
                             reactions.getConstants(j), false);
      // add term to rhs
      r.append(QString(" + (%1)").arg(sym.simplify().c_str()));
    }
    // reparse full rhs to simplify
    SPDLOG_DEBUG("Species {} Reparsing all reaction terms", speciesIDs.at(i));
    // parse expression with symengine to simplify
    symbolic::Symbolic sym(r.toStdString(), speciesIDs, {}, false);
    // if provided, relabel species with relabelledSpeciesIDs
    const auto *outputSpecies = &speciesIDs;
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
static std::optional<std::size_t>
getSpeciesIndex(const model::Model *doc, const std::string &speciesID,
                const std::vector<std::string> &speciesIDs) {
  if (auto it = std::find(speciesIDs.cbegin(), speciesIDs.cend(), speciesID);
      it != speciesIDs.cend() &&
      doc->getSpecies().isReactive(speciesID.c_str())) {
    return static_cast<std::size_t>(it - speciesIDs.cbegin());
  }
  return {};
}

std::vector<double>
Reaction::getStoichMatrixRow(const model::Model *doc,
                             const std::string &reacId) const {
  std::vector<double> matrixRow(speciesIDs.size(), 0);
  bool isReaction = false;
  for (const auto &speciesID : speciesIDs) {
    auto speciesName = doc->getSpecies().getName(speciesID.c_str());
    auto stoichCoeff = doc->getReactions().getSpeciesStoichiometry(
        reacId.c_str(), speciesID.c_str());
    SPDLOG_DEBUG("product '{}'", speciesID);
    auto speciesIndex = getSpeciesIndex(doc, speciesID, speciesIDs);
    if (speciesIndex) {
      SPDLOG_DEBUG("  -> stoich coeff[{}]: {}", speciesIndex.value(),
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

const std::vector<std::pair<std::string, double>> &
Reaction::getConstants(std::size_t reactionIndex) const {
  return constants.at(reactionIndex);
}

Reaction::Reaction(const model::Model *doc, std::vector<std::string> species,
                   const std::vector<std::string> &reactionIDs)
    : speciesIDs(std::move(species)) {
  // RateRule is f(...) for a species s: ds/dt = f(...)
  // Species which have a raterule cannot be created or destroyed in any kinetic
  // reactions
  // todo: not currently valid if raterule involves species in multiple
  // compartments
  for (std::size_t sIndex = 0; sIndex < speciesIDs.size(); ++sIndex) {
    auto ruleExpr = doc->getRateRule(speciesIDs[sIndex]);
    if (!ruleExpr.empty()) {
      std::vector<double> Mrow(speciesIDs.size(), 0);
      Mrow[sIndex] = 1.0;
      M.push_back(Mrow);
      expressions.push_back(ruleExpr);
      constants.emplace_back();
      for (const auto &[id, value] :
           doc->getParameters().getGlobalConstants()) {
        constants.back().push_back({id, value});
      }
      SPDLOG_INFO("adding rate rule for species {}", speciesIDs[sIndex]);
      SPDLOG_INFO("  - expr: {}", ruleExpr);
    }
  }

  // process each reaction
  for (const auto &reacID : reactionIDs) {
    // construct row of stoichiometric coefficients for each
    // species produced and consumed by this reaction
    auto row = getStoichMatrixRow(doc, reacID);
    if (!row.empty()) {
      // if matrix row is non-zero, i.e. reaction does something, then insert
      // it into the M matrix, and construct the corresponding reaction term
      M.push_back(row);

      // get local parameters, append to global constants
      constants.emplace_back();
      for (const auto &[id, value] :
           doc->getParameters().getGlobalConstants()) {
        constants.back().push_back({id, value});
      }
      for (const auto &paramId :
           doc->getReactions().getParameterIds(reacID.c_str())) {
        double value =
            doc->getReactions().getParameterValue(reacID.c_str(), paramId);
        constants.back().push_back({paramId.toStdString(), value});
      }

      // construct expression and add to reactions
      auto inlinedExpr = doc->inlineExpr(
          doc->getReactions().getRateExpression(reacID.c_str()).toStdString());
      expressions.push_back(inlinedExpr);
      SPDLOG_INFO("adding reaction {}", reacID);
      SPDLOG_INFO("  - stoichiometric matrix row: {}",
                  utils::vectorToString(row));
      SPDLOG_INFO("  - expr: {}", inlinedExpr);
    }
  }
}

} // namespace simulate
