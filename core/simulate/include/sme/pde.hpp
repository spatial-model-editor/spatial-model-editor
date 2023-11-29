// Partial Differential Equation (PDE) class
//  - construct PDE for given compartment or membrane
//  - constructs PDE reaction terms:
//  R(speciesScaleFactor*species_vector)*reactionScaleFactor
//  - also Jacobian of reaction terms for each species
//  - factor to rescale species
//  - factor to rescale reaction
// Reaction class
//  - construct matrix of stoich coefficients and reaction terms as strings
//  - along with a map of constants

#pragma once

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sme {

namespace model {
class Model;
}

namespace simulate {

class PdeError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct PdeScaleFactors {
  double species{1.0};
  double reaction{1.0};
};

class Pde {
private:
  std::vector<std::string> rhs;
  std::vector<std::vector<std::string>> jacobian;

public:
  explicit Pde(
      const model::Model *doc_ptr, const std::vector<std::string> &speciesIDs,
      const std::vector<std::string> &reactionIDs,
      const std::vector<std::string> &relabelledSpeciesIDs = {},
      const PdeScaleFactors &pdeScaleFactors = {},
      const std::vector<std::string> &extraVariables = {},
      const std::vector<std::string> &relabelledExtraVariables = {},
      const std::map<std::string, double, std::less<>> &substitutions = {});
  [[nodiscard]] const std::vector<std::string> &getRHS() const;
  [[nodiscard]] const std::vector<std::vector<std::string>> &
  getJacobian() const;
};

class Reaction {
private:
  // vector of speciesIDs
  std::vector<std::string> speciesIDs;
  // vector of reaction expressions as strings
  std::vector<std::string> expressions;
  // matrix M_ij of stoichiometric coefficients
  // i is the species index
  // j is the reaction index
  std::vector<std::vector<double>> M;
  // vector of maps of constants
  std::vector<std::vector<std::pair<std::string, double>>> constants;
  std::vector<double> getStoichMatrixRow(const model::Model *doc,
                                         const std::string &reacId) const;

public:
  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] const std::vector<std::string> &getSpeciesIDs() const;
  [[nodiscard]] const std::string &
  getExpression(std::size_t reactionIndex) const;
  [[nodiscard]] double getMatrixElement(std::size_t speciesIndex,
                                        std::size_t reactionIndex) const;
  [[nodiscard]] const std::vector<std::pair<std::string, double>> &
  getConstants(std::size_t reactionIndex) const;
  Reaction(const model::Model *doc, std::vector<std::string> species,
           const std::vector<std::string> &reactionIDs);
};

} // namespace simulate

} // namespace sme
