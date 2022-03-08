#include "optimize_impl.hpp"

namespace sme::simulate {

void applyParameters(const pagmo::vector_double &values,
                     const std::vector<OptParam> &optParams,
                     sme::model::Model *model) {
  for (std::size_t i = 0; i < values.size(); ++i) {
    const auto &param{optParams[i]};
    double value{values[i]};
    switch (param.optParamType) {
    case OptParamType::ModelParameter:
      SPDLOG_INFO("Setting parameter '{}' to {}", param.id.toStdString(),
                  value);
      model->getParameters().setExpression(param.id,
                                           common::dblToQStr(value, 17));
      break;
    case OptParamType::ReactionParameter:
      SPDLOG_INFO("Setting reaction parameter '{}' of reaction '{}' to {}",
                  param.id.toStdString(), param.parentId.toStdString(), value);
      model->getReactions().setParameterValue(param.parentId, param.id, value);
      break;
    default:
      throw std::invalid_argument("Optimization: Invalid OptParamType");
    }
  }
}

double calculateCosts(const std::vector<OptCost> &optCosts,
                      const sme::simulate::Simulation &sim) {
  double cost{0};
  for (const auto &optCost : optCosts) {
    std::vector<double> values;
    auto compIndex{optCost.compartmentIndex};
    auto specIndex{optCost.speciesIndex};
    switch (optCost.optCostType) {
    case OptCostType::Concentration:
      values =
          sim.getConc(sim.getTimePoints().size() - 1, compIndex, specIndex);
      break;
    case OptCostType::ConcentrationDcdt:
      values = sim.getDcdt(compIndex, specIndex);
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
    cost += optCost.scaleFactor * sme::common::sum(values);
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

void PagmoUDP::init(const std::string &xml, const OptimizeOptions &options) {
  SPDLOG_INFO("initializing model");
  xmlModel = xml;
  optimizeOptions = options;
  model = std::make_unique<sme::model::Model>();
  model->importSBMLString(xmlModel);
}

[[nodiscard]] pagmo::vector_double
PagmoUDP::fitness(const pagmo::vector_double &dv) const {
  model->getSimulationData().clear();
  applyParameters(dv, optimizeOptions.optParams, model.get());
  sme::simulate::Simulation sim(*model);
  sim.doTimesteps(optimizeOptions.simulationTime);
  auto cost{calculateCosts(optimizeOptions.optCosts, sim)};
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
