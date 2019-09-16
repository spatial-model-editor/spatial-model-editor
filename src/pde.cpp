#include "pde.hpp"

#include <QString>

#include "logger.hpp"
#include "reactions.hpp"
#include "symbolic.hpp"

namespace pde {

PDE::PDE(const sbml::SbmlDocWrapper *doc_ptr,
         const std::vector<std::string> &speciesIDs,
         const std::vector<std::string> &reactionIDs) {
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
      SPDLOG_DEBUG("Species {} Reaction {} = {}", speciesIDs.at(i), j, expr);
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
    rhs.push_back(sym.simplify());
    for (const auto &s : speciesIDs) {
      jacobian.back().push_back(sym.diff(s));
    }
  }
}

const std::vector<std::string> &PDE::getRHS() const { return rhs; }
const std::vector<std::vector<std::string>> &PDE::getJacobian() const {
  return jacobian;
}

}  // namespace pde
