#pragma once
#include "sme/model.hpp"
#include "sme/optimize_options.hpp"
#include "sme/simulate.hpp"
#include <memory>
#include <pagmo/algorithm.hpp>
#include <pagmo/archipelago.hpp>
#include <pagmo/population.hpp>
// Qt defines emit keyword which interferes with a tbb emit() function
#ifdef emit
#undef emit
#include <oneapi/tbb/concurrent_queue.h>
#define emit // restore the Qt empty definition of "emit"
#else
#include <oneapi/tbb/concurrent_queue.h>
#endif

namespace sme::simulate {

using ThreadsafeModelQueue =
    oneapi::tbb::concurrent_queue<std::shared_ptr<sme::model::Model>>;

struct OptTimestep {
  // the time to simulate for
  double simulationTime;
  // the indices of the OptCosts to calculate after simulating
  std::vector<std::size_t> optCostIndices;
};

/**
 * @brief Optimize model parameters
 *
 * Optimizes the supplied model parameters to minimize the supplied cost
 * functions.
 */
class Optimization {
private:
  std::unique_ptr<pagmo::algorithm> algo{nullptr};
  std::unique_ptr<pagmo::archipelago> archi{nullptr};
  std::unique_ptr<OptimizeOptions> optimizeOptions{nullptr};
  std::unique_ptr<std::vector<OptTimestep>> optTimesteps{nullptr};
  std::unique_ptr<std::string> xmlModel{nullptr};
  std::atomic<bool> isRunning{false};
  std::atomic<bool> stopRequested{false};
  std::atomic<std::size_t> nIterations{0};
  std::vector<double> bestFitness;
  std::vector<std::vector<double>> bestParams;
  std::unique_ptr<ThreadsafeModelQueue> modelQueue{nullptr};

public:
  /**
   * @brief Constructs an Optimization object from the supplied model
   *
   * @param[in] model the model to optimize
   */
  explicit Optimization(sme::model::Model &model);
  /**
   * @brief Do n iterations of parameter optimization
   */
  std::size_t evolve(std::size_t n = 1);
  /**
   * @brief Apply the current best parameter values to the supplied model
   */
  void applyParametersToModel(sme::model::Model *model) const;
  /**
   * @brief The best set of parameters from each iteration
   */
  [[nodiscard]] const std::vector<std::vector<double>> &getParams() const;
  /**
   * @brief The names of the parameters being optimized
   */
  [[nodiscard]] std::vector<QString> getParamNames() const;
  /**
   * @brief The best fitness from each iteration
   */
  [[nodiscard]] const std::vector<double> &getFitness() const;
  /**
   * @brief The number of completed evolve iterations
   */
  [[nodiscard]] std::size_t getIterations() const;
  /**
   * @brief True if the optimization is currently running
   */
  [[nodiscard]] bool getIsRunning() const;
  /**
   * @brief True if requestStop() has been called
   */
  [[nodiscard]] bool getIsStopping() const;
  /**
   * @brief Stop the evolution as soon as possible
   */
  void requestStop();
};

} // namespace sme::simulate
