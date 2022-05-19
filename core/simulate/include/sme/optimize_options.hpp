#pragma once
#include <QString>
#include <vector>

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
   * @brief The name of this OptParam as displayed to the user
   */
  QString name;
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
   * @brief The name of this OptCost as displayed to the user
   */
  QString name;
  /**
   * @brief The id of the species in the model
   */
  QString id;
  /**
   * @brief The simulation time at which the cost function should be calculated
   */
  double simulationTime{1.0};
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
   * @brief The target values to compare with the species concentration or dcdt
   *
   * Should contain a value for each pixel in the image,
   * including those outside of the compartment the species is located in.
   * If empty, the target values are assumed to be zero everywhere.
   */
  std::vector<double> targetValues;
  /**
   * @brief A small number to avoid dividing by zero in relative differences
   *
   * A small value to be added to the denominator of relative differences to
   * avoid numerical issues caused by dividing by zero.
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
  std::size_t nParticles{2};
  /**
   * @brief The parameters to optimize
   */
  std::vector<OptParam> optParams;
  /**
   * @brief The costs to minimize
   */
  std::vector<OptCost> optCosts;
};

} // namespace sme::simulate
