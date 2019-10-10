#include "pde.hpp"

#include <QString>

#include "logger.hpp"
#include "reactions.hpp"
#include "sbml.hpp"
#include "symbolic.hpp"

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
  reactions::Reaction reactions(doc_ptr, speciesIDs, reactionIDs);

  // construct symbolic expressions: one rhs + Jacobian for each species
  rhs.clear();
  jacobian.clear();
  for (std::size_t i = 0; i < speciesIDs.size(); ++i) {
    jacobian.emplace_back();
    QString r("0.0");
    for (std::size_t j = 0; j < reactions.reacExpressions.size(); ++j) {
      // get reaction term
      QString expr = QString("%1*(%2) ")
                         .arg(QString::number(reactions.M.at(j).at(i), 'g', 18),
                              reactions.reacExpressions.at(j).c_str());
      // rescale by supplied reactionScaleFactor
      QString scaleFactor("1");
      if (reactionScaleFactors.size() == reactionIDs.size()) {
        scaleFactor = reactionScaleFactors[j].c_str();
      }
      expr = QString("((%1)/%2) ").arg(expr, scaleFactor);
      SPDLOG_DEBUG("Species {} Reaction {} = {}", speciesIDs.at(i), j,
                   expr.toStdString());
      // parse and inline constants
      symbolic::Symbolic sym(expr.toStdString(), reactions.speciesIDs,
                             reactions.constants[j]);
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

}  // namespace pde
