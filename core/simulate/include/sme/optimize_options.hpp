#pragma once
#include <array>
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
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
  std::string name;
  /**
   * @brief The id of the parameter in the model
   */
  std::string id;
  /**
   * @brief The id of the parent of the parameter in the model (optional)
   */
  std::string parentId;
  /**
   * @brief The lower bound on the allowed values of the parameter
   */
  double lowerBound;
  /**
   * @brief The upper bound on the allowed values of the parameter
   */
  double upperBound;

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(optParamType), CEREAL_NVP(name), CEREAL_NVP(id),
         CEREAL_NVP(parentId), CEREAL_NVP(lowerBound), CEREAL_NVP(upperBound));
    }
  }
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
 * @brief Defines a cost function to be minimized
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
  std::string name;
  /**
   * @brief The id of the species in the model
   */
  std::string id;
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
  double weight{1.0};
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

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(optCostType), CEREAL_NVP(optCostDiffType), CEREAL_NVP(name),
         CEREAL_NVP(id), CEREAL_NVP(simulationTime), CEREAL_NVP(weight),
         CEREAL_NVP(compartmentIndex), CEREAL_NVP(speciesIndex),
         CEREAL_NVP(targetValues), CEREAL_NVP(epsilon));
    }
  }
};

/**
 * @brief Types of algorithms that can be used in optimization
 */
enum class OptAlgorithmType {
  PSO,
  GPSO,
  DE,
  iDE,
  jDE,
  pDE,
  ABC,
  gaco,
  COBYLA,
  BOBYQA,
  NMS,
  sbplx,
  AL,
  PRAXIS
};

/**
 * @brief An array of all algorithm types for iterating over
 */
inline constexpr std::array<OptAlgorithmType, 14> optAlgorithmTypes{
    OptAlgorithmType::PSO,    OptAlgorithmType::GPSO,  OptAlgorithmType::DE,
    OptAlgorithmType::iDE,    OptAlgorithmType::jDE,   OptAlgorithmType::pDE,
    OptAlgorithmType::ABC,    OptAlgorithmType::gaco,  OptAlgorithmType::COBYLA,
    OptAlgorithmType::BOBYQA, OptAlgorithmType::NMS,   OptAlgorithmType::sbplx,
    OptAlgorithmType::AL,     OptAlgorithmType::PRAXIS};

std::string toString(sme::simulate::OptAlgorithmType optAlgorithmType);

/**
 * @brief Optimization algorithm options
 */
struct OptAlgorithm {
  /**
   * @brief The algorithm to use
   */
  OptAlgorithmType optAlgorithmType{OptAlgorithmType::PSO};
  /**
   * @brief The number of islands
   */
  std::size_t islands{1};
  /**
   * @brief The population size in each island
   */
  std::size_t population{2};

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(optAlgorithmType), CEREAL_NVP(islands),
         CEREAL_NVP(population));
    }
  }
};

/**
 * @brief Optimization options
 */
struct OptimizeOptions {
  /**
   * @brief The algorithm to use
   */
  OptAlgorithm optAlgorithm;
  /**
   * @brief The parameters to optimize
   */
  std::vector<OptParam> optParams;
  /**
   * @brief The costs to minimize
   */
  std::vector<OptCost> optCosts;

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(optAlgorithm), CEREAL_NVP(optParams), CEREAL_NVP(optCosts));
    }
  }
};

} // namespace sme::simulate

CEREAL_CLASS_VERSION(sme::simulate::OptimizeOptions, 0);
CEREAL_CLASS_VERSION(sme::simulate::OptCost, 0);
CEREAL_CLASS_VERSION(sme::simulate::OptParam, 0);
CEREAL_CLASS_VERSION(sme::simulate::OptAlgorithm, 0);
