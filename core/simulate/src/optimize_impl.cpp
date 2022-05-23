#include "optimize_impl.hpp"
#include "sme/logger.hpp"

namespace sme::simulate {

void applyParameters(const pagmo::vector_double &values,
                     const std::vector<OptParam> &optParams,
                     sme::model::Model *model) {
  for (std::size_t i = 0; i < values.size(); ++i) {
    const auto &param{optParams[i]};
    double value{values[i]};
    switch (param.optParamType) {
    case OptParamType::ModelParameter:
      SPDLOG_INFO("Setting parameter '{}' to {}", param.id, value);
      model->getParameters().setExpression(param.id.c_str(),
                                           common::dblToQStr(value, 17));
      break;
    case OptParamType::ReactionParameter:
      SPDLOG_INFO("Setting reaction parameter '{}' of reaction '{}' to {}",
                  param.id, param.parentId, value);
      model->getReactions().setParameterValue(param.parentId.c_str(),
                                              param.id.c_str(), value);
      break;
    default:
      throw std::invalid_argument("Optimization: Invalid OptParamType");
    }
  }
}

double calculateCosts(const std::vector<OptCost> &optCosts,
                      const std::vector<std::size_t> &optCostIndices,
                      const sme::simulate::Simulation &sim) {
  double cost{0};
  for (const auto &optCostIndex : optCostIndices) {
    const auto &optCost{optCosts[optCostIndex]};
    std::vector<double> values;
    auto compIndex{optCost.compartmentIndex};
    auto specIndex{optCost.speciesIndex};
    switch (optCost.optCostType) {
    case OptCostType::Concentration:
      values = sim.getConcArray(sim.getTimePoints().size() - 1, compIndex,
                                specIndex);
      break;
    case OptCostType::ConcentrationDcdt:
      values = sim.getDcdtArray(compIndex, specIndex);
      break;
    default:
      throw std::invalid_argument("Optimization: Invalid OptCostType");
    }
    if (optCost.targetValues.empty()) {
      // default target is zero if not specified
      for (auto &value : values) {
        value = std::abs(value);
      }
    } else {
      if (values.size() != optCost.targetValues.size()) {
        SPDLOG_ERROR(
            "Mismatch between size of values ({}) and target values ({})",
            values.size(), optCost.targetValues.size());
        throw std::invalid_argument(
            "Optimization: Target values size mismatch");
      }
      for (std::size_t i = 0; i < values.size(); ++i) {
        double diff{values[i] - optCost.targetValues[i]};
        if (optCost.optCostDiffType == OptCostDiffType::Relative) {
          diff /= (std::abs(optCost.targetValues[i]) + optCost.epsilon);
        }
        values[i] = std::abs(diff);
      }
    }
    cost += optCost.weight * sme::common::sum(values);
  }
  return cost;
}

PagmoUDP::PagmoUDP() = default;

PagmoUDP::PagmoUDP(PagmoUDP &&) noexcept = default;

PagmoUDP &PagmoUDP::operator=(PagmoUDP &&) noexcept = default;

PagmoUDP::~PagmoUDP() = default;

PagmoUDP &PagmoUDP::operator=(const PagmoUDP &other) {
  if (&other != this) {
    init(other.xmlModel, other.optimizeOptions);
  }
  return *this;
}

PagmoUDP::PagmoUDP(const PagmoUDP &other) {
  init(other.xmlModel, other.optimizeOptions);
}

static std::vector<OptTimestep>
getOptTimesteps(const OptimizeOptions &options) {
  std::vector<OptTimestep> optTimesteps;
  // get time for each optCost
  std::vector<double> times;
  for (const auto &optCost : options.optCosts) {
    times.push_back(optCost.simulationTime);
  }
  // sort times
  std::vector sortedUniqueTimes{times};
  std::sort(sortedUniqueTimes.begin(), sortedUniqueTimes.end());
  // margin within which times are considered equal:
  constexpr double relativeEps{1e-13};
  double epsilon{sortedUniqueTimes.back() * relativeEps};
  // remove (approx) duplicates
  sortedUniqueTimes.erase(std::unique(sortedUniqueTimes.begin(),
                                      sortedUniqueTimes.end(),
                                      [epsilon](double a, double b) {
                                        return std::abs(a - b) < epsilon;
                                      }),
                          sortedUniqueTimes.end());
  double previousTime{0};
  for (double sortedUniqueTime : sortedUniqueTimes) {
    double dt{sortedUniqueTime - previousTime};
    previousTime += dt;
    optTimesteps.push_back({dt, {}});
    for (std::size_t i = 0; i < times.size(); ++i) {
      if (std::abs(times[i] - sortedUniqueTime) < epsilon) {
        optTimesteps.back().optCostIndices.push_back(i);
      }
    }
  }
  for (const auto &optTimestep : optTimesteps) {
    SPDLOG_INFO("t = {}", optTimestep.simulationTime);
    for (const auto optCostIndex : optTimestep.optCostIndices) {
      SPDLOG_INFO("  - index {}", optCostIndex);
    }
  }
  return optTimesteps;
}

void PagmoUDP::init(const std::string &xml, const OptimizeOptions &options) {
  SPDLOG_INFO("initializing model");
  xmlModel = xml;
  optimizeOptions = options;
  if (optimizeOptions.optCosts.empty()) {
    throw std::invalid_argument(
        "Optimization: No optimization targets specified");
  }
  if (optimizeOptions.optParams.empty()) {
    throw std::invalid_argument(
        "Optimization: No parameters to optimize specified");
  }
  optTimesteps = getOptTimesteps(options);
  model = std::make_unique<sme::model::Model>();
  model->importSBMLString(xmlModel);
}

[[nodiscard]] pagmo::vector_double
PagmoUDP::fitness(const pagmo::vector_double &dv) const {
  model->getSimulationData().clear();
  applyParameters(dv, optimizeOptions.optParams, model.get());
  sme::simulate::Simulation sim(*model);
  double cost{0.0};
  for (const auto &optTimestep : optTimesteps) {
    sim.doTimesteps(optTimestep.simulationTime);
    cost += calculateCosts(optimizeOptions.optCosts, optTimestep.optCostIndices,
                           sim);
  }
  return {cost};
}

[[nodiscard]] std::pair<pagmo::vector_double, pagmo::vector_double>
PagmoUDP::get_bounds() const {
  std::pair<pagmo::vector_double, pagmo::vector_double> bounds;
  bounds.first.reserve(optimizeOptions.optParams.size());
  bounds.second.reserve(optimizeOptions.optParams.size());
  for (const auto &optParam : optimizeOptions.optParams) {
    bounds.first.push_back(optParam.lowerBound);
    bounds.second.push_back(optParam.upperBound);
  }
  return bounds;
}

} // namespace sme::simulate
