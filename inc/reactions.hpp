// Reaction compilation routines
//  - construct stoich matrix and reaction terms as strings
//  - along with a map of constants

#pragma once

#include <QStringList>
#include <map>

namespace sbml {
class SbmlDocWrapper;
}

namespace reactions {

class Reaction {
 private:
  void init(const sbml::SbmlDocWrapper *doc,
            const std::vector<std::string> &species,
            const std::vector<std::string> &reactionIDs);

 public:
  // vector of reaction expressions as strings
  std::vector<std::string> reacExpressions;
  // matrix M_ij of stoichiometric coefficients
  // i is the species index
  // j is the reaction index
  std::vector<std::vector<double>> M;
  // vector of speciesIDs
  std::vector<std::string> speciesIDs;
  // vector of maps of constants
  std::vector<std::map<std::string, double>> constants;
  Reaction(const sbml::SbmlDocWrapper *doc_ptr,
           const std::vector<std::string> &species,
           const std::vector<std::string> &reactionIDs);
  Reaction(const sbml::SbmlDocWrapper *doc_ptr, const QStringList &species,
           const QStringList &reactionIDs);
};

}  // namespace reactions
