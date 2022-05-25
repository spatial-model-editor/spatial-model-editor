#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/optimize.hpp"
#include "sme/simulate.hpp"
#include "sme/utils.hpp"
#include <iostream>
#include <pagmo/algorithms/pso.hpp>

namespace sme::simulate {

void applyParameters(const pagmo::vector_double &values,
                     sme::model::Model *model);

double calculateCosts(const std::vector<OptCost> &optCosts,
                      const std::vector<std::size_t> &optCostIndices,
                      const sme::simulate::Simulation &sim);

/**
 * @brief Implements a Pagmo User Defined Problem to evolve
 *
 * @note Needs to be (cheaply) copy-constructible, implement `fitness()` and
 * `get_bounds()` functions, and be thread-safe, see
 * https://esa.github.io/pagmo2/docs/cpp/problem.html
 */

class PagmoUDP {
private:
  ThreadsafeModelQueue *modelQueue{nullptr};
  const std::string *xmlModel{nullptr};
  const OptimizeOptions *optimizeOptions{nullptr};
  const std::vector<OptTimestep> *optTimesteps{nullptr};

public:
  PagmoUDP() = default;
  explicit PagmoUDP(const std::string *xmlModel,
                    const OptimizeOptions *optimizeOptions,
                    const std::vector<OptTimestep> *optTimesteps,
                    ThreadsafeModelQueue *modelQueue);
  [[nodiscard]] pagmo::vector_double
  fitness(const pagmo::vector_double &dv) const;
  [[nodiscard]] std::pair<pagmo::vector_double, pagmo::vector_double>
  get_bounds() const;
};

} // namespace sme::simulate
