#include "optimize_impl.hpp"
#include "sme/logger.hpp"

namespace sme::simulate {

void applyParameters(const pagmo::vector_double &values,
                     sme::model::Model *model) {
  const auto &optParams{model->getOptimizeOptions().optParams};
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

PagmoUDP::PagmoUDP(const std::string *xmlModel,
                   const OptimizeOptions *optimizeOptions,
                   const std::vector<OptTimestep> *optTimesteps,
                   ThreadsafeModelQueue *modelQueue,
                   const sme::simulate::Optimization *optimization)
    : xmlModel{xmlModel}, optimizeOptions{optimizeOptions},
      optTimesteps{optTimesteps}, modelQueue{modelQueue}, optimization{
                                                              optimization} {}

[[nodiscard]] pagmo::vector_double
PagmoUDP::fitness(const pagmo::vector_double &dv) const {
  std::shared_ptr<sme::model::Model> m;
  if (optimization->getIsStopping()) {
    return {std::numeric_limits<double>::max()};
  }
  if (modelQueue == nullptr || !modelQueue->try_pop(m)) {
    SPDLOG_INFO("model queue missing or empty: constructing model");
    m = std::make_shared<sme::model::Model>();
    m->importSBMLString(*xmlModel);
  }
  m->getSimulationData().clear();
  applyParameters(dv, m.get());
  sme::simulate::Simulation sim(*m);
  double cost{0.0};
  for (const auto &optTimestep : *optTimesteps) {
    sim.doMultipleTimesteps({{1, optTimestep.simulationTime}}, -1,
                            [this]() { return optimization->getIsStopping(); });
    if (optimization->getIsStopping()) {
      return {std::numeric_limits<double>::max()};
    }
    cost += calculateCosts(optimizeOptions->optCosts,
                           optTimestep.optCostIndices, sim);
  }
  if (modelQueue != nullptr) {
    modelQueue->push(std::move(m));
  }
  return {cost};
}

[[nodiscard]] std::pair<pagmo::vector_double, pagmo::vector_double>
PagmoUDP::get_bounds() const {
  std::pair<pagmo::vector_double, pagmo::vector_double> bounds;
  bounds.first.reserve(optimizeOptions->optParams.size());
  bounds.second.reserve(optimizeOptions->optParams.size());
  for (const auto &optParam : optimizeOptions->optParams) {
    bounds.first.push_back(optParam.lowerBound);
    bounds.second.push_back(optParam.upperBound);
  }
  return bounds;
}

} // namespace sme::simulate
