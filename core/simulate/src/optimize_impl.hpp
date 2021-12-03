#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/optimize.hpp"
#include "sme/simulate.hpp"
#include "sme/utils.hpp"
#include <iostream>
#include <pagmo/algorithms/pso.hpp>

namespace sme::simulate {

void applyParameters(const pagmo::vector_double &values,
                     const std::vector<OptParam> &optParams,
                     sme::model::Model *model);

double calculateCosts(const std::vector<OptCost> &optCosts,
                      const sme::simulate::Simulation &sim);

/**
 * @brief Implements a Pagmo User Defined Problem to evolve
 *
 * @note Needs to be copy-constructible, implement fitness() and get_bounds()
 * functions and be thread-safe, see
 * https://esa.github.io/pagmo2/docs/cpp/problem.html
 *
 * @note Pagmo also assumes a UDP is cheap to copy, in particular when using
 * Archipelagos for parallel evolution many copies are made. Currently ours is
 * not cheap to copy as it also makes a copy of the model (in order to be thread
 * safe when modifying/simulating this model).
 */

class PagmoUDP {
private:
  std::string xmlModel;
  std::unique_ptr<sme::model::Model> model;
  OptimizeOptions optimizeOptions;

public:
  explicit PagmoUDP();
  PagmoUDP(PagmoUDP &&) noexcept;
  PagmoUDP &operator=(PagmoUDP &&) noexcept;
  PagmoUDP &operator=(const PagmoUDP &other);
  PagmoUDP(const PagmoUDP &other);
  ~PagmoUDP();
  void init(const std::string &xml, const OptimizeOptions &options);
  [[nodiscard]] pagmo::vector_double
  fitness(const pagmo::vector_double &dv) const;
  [[nodiscard]] std::pair<pagmo::vector_double, pagmo::vector_double>
  get_bounds() const;
};

} // namespace sme::simulate
