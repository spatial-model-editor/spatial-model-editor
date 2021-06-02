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

namespace sme::simulate {

Pde::Pde(const model::Model *doc_ptr,
         const std::vector<std::string> &speciesIDs,
         const std::vector<std::string> &reactionIDs,
         const std::vector<std::string> &relabelledSpeciesIDs,
         const PdeScaleFactors &pdeScaleFactors,
         const std::vector<std::string> &extraVariables,
         const std::vector<std::string> &relabelledExtraVariables,
         const std::map<std::string, double, std::less<>> &substitutions) {
  bool relabel{!relabelledSpeciesIDs.empty() ||
               !relabelledExtraVariables.empty()};
  if (relabel && relabelledSpeciesIDs.size() != speciesIDs.size()) {
    SPDLOG_WARN("Ignoring relabelling: relabelledSpecies "
                "size {} does not match number of species {}",
                relabelledSpeciesIDs.size(), speciesIDs.size());
    relabel = false;
  }
  if (relabel && relabelledExtraVariables.size() != extraVariables.size()) {
    SPDLOG_WARN("Ignoring relabelling: relabelledExtraVariables "
                "size {} does not match number of extraVariables {}",
                relabelledExtraVariables.size(), extraVariables.size());
    relabel = false;
  }
  SPDLOG_INFO("Species rescaled by factor: {}", pdeScaleFactors.species);
  SPDLOG_INFO("Reactions rescaled by factor: {}", pdeScaleFactors.reaction);
  // construct reaction expressions and stoich matrix
  Reaction reactions(doc_ptr, speciesIDs, reactionIDs);

  // construct symbolic expressions: one rhs + Jacobian for each species
  rhs.clear();
  jacobian.clear();
  for (std::size_t i = 0; i < speciesIDs.size(); ++i) {
    jacobian.emplace_back();
    QString r("0.0");
    auto vars{reactions.getSpeciesIDs()};
    vars.insert(vars.end(), extraVariables.cbegin(), extraVariables.cend());
    for (std::size_t j = 0; j < reactions.size(); ++j) {
      // get reaction term
      QString expr =
          QString("%1*(%2) ")
              .arg(utils::dblToQStr(reactions.getMatrixElement(j, i)),
                   reactions.getExpression(j).c_str());
      // rescale by supplied reactionScaleFactor
      auto str = fmt::format("{}", pdeScaleFactors.reaction);
      expr = QString("((%1)*(%2)) ").arg(expr, str.c_str());
      SPDLOG_DEBUG("Species {} Reaction {} = {}", speciesIDs.at(i), j,
                   expr.toStdString());
      auto constants{reactions.getConstants(j)};
      if (!substitutions.empty()) {
        // substitute values of any constants in substitutions map
        for (auto &[id, v] : constants) {
          if (auto iter = substitutions.find(id); iter != substitutions.end()) {
            SPDLOG_INFO("Substituting: {} = {} -> {}", id, v, iter->second);
            v = iter->second;
          }
        }
      }
      // parse and inline constants & function calls
      utils::Symbolic sym(expr.toStdString(), vars, constants,
                          doc_ptr->getFunctions().getSymbolicFunctions(),
                          false);
      if (!sym.isValid()) {
        throw PdeError(sym.getErrorMessage());
      }
      // add term to rhs
      r.append(QString(" + (%1)").arg(sym.inlinedExpr().c_str()));
    }
    // reparse full rhs to simplify
    SPDLOG_DEBUG("Species {} Reparsing all reaction terms", speciesIDs.at(i));
    // parse expression with symengine to simplify
    utils::Symbolic sym(r.toStdString(), vars, {}, {}, false);
    // rescale species (but not the extra variables)
    SPDLOG_DEBUG("rescaling species");
    sym.rescale(pdeScaleFactors.species, extraVariables);
    auto outputSpecies = speciesIDs;
    if (relabel) {
      SPDLOG_DEBUG("re-labelling species");
      outputSpecies = relabelledSpeciesIDs;
      outputSpecies.insert(outputSpecies.end(),
                           relabelledExtraVariables.cbegin(),
                           relabelledExtraVariables.cend());
      sym.relabel(outputSpecies);
    }
    for (const auto &s : outputSpecies) {
      jacobian.back().push_back(sym.diff(s));
    }
    rhs.push_back(sym.inlinedExpr());
  }
}

const std::vector<std::string> &Pde::getRHS() const { return rhs; }
const std::vector<std::vector<std::string>> &Pde::getJacobian() const {
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
    double stoichCoeff{doc->getReactions().getSpeciesStoichiometry(
        reacId.c_str(), speciesID.c_str())};
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
  // process each reaction
  for (const auto &reacID : reactionIDs) {
    // construct row of stoichiometric coefficients for each
    // species produced and consumed by this reaction
    auto row = getStoichMatrixRow(doc, reacID);
    if (!row.empty()) {
      // if matrix row is non-zero, i.e. reaction does something, then
      // insert it into the M matrix, and construct the corresponding
      // reaction term
      M.push_back(row);

      // get local parameters, append to global constants
      constants.emplace_back();
      for (const auto &c : doc->getParameters().getGlobalConstants()) {
        constants.back().push_back({c.id, c.value});
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

} // namespace sme::simulate
