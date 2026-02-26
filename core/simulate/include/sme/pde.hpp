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

/**
 * @brief Error type for PDE/reaction construction failures.
 */
class PdeError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

/**
 * @brief Scaling factors used when forming PDE expressions.
 */
struct PdeScaleFactors {
  /**
   * @brief Species variable scale.
   */
  double species{1.0};
  /**
   * @brief Reaction expression scale.
   */
  double reaction{1.0};
};

/**
 * @brief Symbolic PDE rhs and Jacobian for a compartment/membrane system.
 */
class Pde {
private:
  std::vector<std::string> rhs;
  std::vector<std::vector<std::string>> jacobian;

public:
  /**
   * @brief Construct PDE from model, species set, and reaction set.
   */
  explicit Pde(
      const model::Model *doc_ptr, const std::vector<std::string> &speciesIDs,
      const std::vector<std::string> &reactionIDs,
      const std::vector<std::string> &relabelledSpeciesIDs = {},
      const PdeScaleFactors &pdeScaleFactors = {},
      const std::vector<std::string> &extraVariables = {},
      const std::vector<std::string> &relabelledExtraVariables = {},
      const std::map<std::string, double, std::less<>> &substitutions = {});
  /**
   * @brief Right-hand-side expressions.
   */
  [[nodiscard]] const std::vector<std::string> &getRHS() const;
  /**
   * @brief Jacobian expressions.
   */
  [[nodiscard]] const std::vector<std::vector<std::string>> &
  getJacobian() const;
};

/**
 * @brief Reaction expressions, stoichiometry matrix, and constants.
 */
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
  /**
   * @brief Number of reactions.
   */
  [[nodiscard]] std::size_t size() const;
  /**
   * @brief Species ids used in reaction system.
   */
  [[nodiscard]] const std::vector<std::string> &getSpeciesIDs() const;
  /**
   * @brief Reaction expression by index.
   */
  [[nodiscard]] const std::string &
  getExpression(std::size_t reactionIndex) const;
  /**
   * @brief Stoichiometric matrix element ``M(species,reaction)``.
   */
  [[nodiscard]] double getMatrixElement(std::size_t speciesIndex,
                                        std::size_t reactionIndex) const;
  /**
   * @brief Constant substitutions associated with reaction.
   */
  [[nodiscard]] const std::vector<std::pair<std::string, double>> &
  getConstants(std::size_t reactionIndex) const;
  /**
   * @brief Construct reaction helper from model and ids.
   */
  Reaction(const model::Model *doc, std::vector<std::string> species,
           const std::vector<std::string> &reactionIDs);
};

} // namespace simulate

} // namespace sme
