#pragma once
#include "sme/model.hpp"
#include "sme/simulate.hpp"
#include <pagmo/algorithm.hpp>
#include <pagmo/population.hpp>
#include <pagmo/problem.hpp>

namespace sme::simulate {

/**
 * @brief Types of model parameters that can be used in optimization
 */
enum class OptParamType { ModelParameter, ReactionParameter };

/**
 * @brief Defines a parameter to be used in optimization
 */
struct OptParam {
  /**
   * @brief The type of parameter
   */
  OptParamType optParamType;
  /**
   * @brief The id of the parameter in the model
   */
  QString id;
  /**
   * @brief The id of the parent of the parameter in the model (optional)
   */
  QString parentId;
  /**
   * @brief The lower bound on the allowed values of the parameter
   */
  double lowerBound;
  /**
   * @brief The upper bound on the allowed values of the parameter
   */
  double upperBound;
};

/**
 * @brief Types of costs that can be used in optimization
 */
enum class OptCostType { Concentration, ConcentrationDcdt };

/**
 * @brief Types of differences that can be used in costs
 */
enum class OptCostDiffType { Absolute, Relative };

/**
 * @brief Defines a cost function to be minimzed
 */
struct OptCost {
  /**
   * @brief The type of cost function
   */
  OptCostType optCostType;
  /**
   * @brief The type of difference (e.g. absolute, relative) used in the cost
   * function
   */
  OptCostDiffType optCostDiffType;
  /**
   * @brief The scale factor to multiply the cost by
   *
   * This assigns a relative weight to this cost when the optimization involves
   * the sum of multiple cost functions.
   */
  double scaleFactor{1.0};
  /**
   * @brief The index of the compartment containing the species
   */
  std::size_t compartmentIndex;
  /**
   * @brief The index of the species
   */
  std::size_t speciesIndex;
  /**
   * @brief The target values to compare with the species concentration
   *
   * Should contain a desired value for each pixel in the compartment.
   * If empty, the target values are assumed to be zero.
   */
  std::vector<double> targetValues;
  /**
   * @brief A small number to avoid dividing by zero in relative differences
   *
   * A small value to be added to the denominator of relative differences to
   * avoid instabilities caused by dividing by zero.
   */
  double epsilon{1e-15};
};

/**
 * @brief Optimization options
 */
struct OptimizeOptions {
  /**
   * @brief The population size to evolve
   */
  std::size_t nParticles{1};
  /**
   * @brief The time for which the model should be simulated
   */
  double simulationTime{1};
  /**
   * @brief The parameters to optimize
   */
  std::vector<OptParam> optParams;
  /**
   * @brief The costs to minimize
   */
  std::vector<OptCost> optCosts;
};

/**
 * @brief Optimize model parameters
 *
 * Optimizes the supplied model parameters to minimize the supplied cost
 * functions.
 */
class Optimization {
private:
  pagmo::problem prob;
  pagmo::algorithm algo;
  pagmo::population pop;
  OptimizeOptions options;
  std::size_t niter{0};

public:
  /**
   * @brief Constructs an Optimization object from the supplied model and
   * optimization options
   *
   * @param[in] model the model to optimize
   * @param[in] optimizeOptions the optimization options
   */
  explicit Optimization(sme::model::Model &model,
                        OptimizeOptions optimizeOptions);
  /**
   * @brief Do a single evolution of parameter optimization
   */
  void evolve();
  /**
   * @brief Apply the current best parameter values to the supplied model
   */
  void applyParametersToModel(sme::model::Model *model) const;
  /**
   * @brief The current best set of parameters
   */
  std::vector<double> params() const;
  /**
   * @brief The fitness of the current best set of parameters
   */
  std::vector<double> fitness() const;
  /**
   * @brief The number of completed evolve iterations
   */
  std::size_t iterations() const;
};

} // namespace sme::simulate
