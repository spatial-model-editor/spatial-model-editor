#pragma once
#include "sme/image_stack.hpp"
#include "sme/model.hpp"
#include "sme/optimize_options.hpp"
#include "sme/simulate.hpp"
#include <memory>
#include <mutex>
#include <optional>
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

struct OptConstData {
  std::string xmlModel{};
  OptimizeOptions optimizeOptions{};
  std::vector<OptTimestep> optTimesteps{};
  common::Volume imageSize{};
  std::vector<double> maxTargetValues{};
};

/**
 * @brief Optimize model parameters
 *
 * Optimizes the supplied model parameters to minimize the supplied cost
 * functions.
 */
class Optimization {
private:
  struct BestResults {
    std::vector<std::vector<double>> values{};
    double fitness{std::numeric_limits<double>::max()};
    std::size_t imageIndex{std::numeric_limits<std::size_t>::max()};
    bool imageChanged{true};
  };
  std::unique_ptr<pagmo::algorithm> algo{nullptr};
  std::unique_ptr<pagmo::archipelago> archi{nullptr};
  std::unique_ptr<OptConstData> optConstData{nullptr};
  std::atomic<bool> isRunning{false};
  std::atomic<bool> stopRequested{false};
  std::atomic<std::size_t> nIterations{0};
  std::vector<double> bestFitness;
  std::vector<std::vector<double>> bestParams;
  mutable std::mutex resultsMutex;
  mutable std::mutex bestResultsMutex;
  BestResults bestResults{};
  std::unique_ptr<ThreadsafeModelQueue> modelQueue{nullptr};
  std::string errorMessage{};

  std::size_t finalizeEvolve(const std::string &newErrorMessage = {});

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
  std::size_t
  evolve(std::size_t n = 1,
         const std::function<void(double, const std::vector<double> &)>
             &callback = {});
  /**
   * @brief Apply the current best parameter values to the supplied model
   */
  bool applyParametersToModel(sme::model::Model *model) const;
  /**
   * @brief The best set of parameters from each iteration
   */
  [[nodiscard]] std::vector<std::vector<double>> getParams() const;
  /**
   * @brief The names of the parameters being optimized
   */
  [[nodiscard]] std::vector<std::string> getParamNames() const;
  /**
   * @brief The best fitness from each iteration
   */
  [[nodiscard]] std::vector<double> getFitness() const;
  /**
   * @brief Try to set a new set of best results for each target
   *
   * The best results are only updated if `fitness` is lower than the current
   * `bestResultsFitness`
   */
  bool setBestResults(double fitness,
                      std::vector<std::vector<double>> &&results);

  /**
   * @brief Get the values for a target obtained with the best currently
   * available parameters.
   *
   */
  [[nodiscard]] std::vector<double>
  getBestResultValues(std::size_t index) const;

  /**
   * @brief Get an image of the a target
   */
  [[nodiscard]] common::ImageStack getTargetImage(std::size_t index) const;

  /**
   * @brief Get the raw values of the optimization target
   */
  [[nodiscard]] const std::vector<double> &
  getTargetValues(std::size_t index) const;

  /**
   * @brief Get an image of the current best result for a target
   */
  [[nodiscard]] std::optional<common::ImageStack>
  getUpdatedBestResultImage(std::size_t index);

  /**
   * @brief Get the current best result for a target
   */
  [[nodiscard]] common::ImageStack getCurrentBestResultImage() const;

  /**
   * @brief Get the size of the domain of the image representing the
   * optimization target in number of pixels in each dimension
   */
  [[nodiscard]] sme::common::Volume getImageSize();

  /**
   * @brief get the maximum value of the target values
   */
  [[nodiscard]] double getMaxValue(std::size_t index);

  /**
   * @brief Get the difference between the optimization target and the current
   * best result
   */
  [[nodiscard]] common::ImageStack getDifferenceImage(std::size_t index);

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
  /**
   * @brief Returns a message if an error occurred - empty if no errors occurred
   */
  const std::string &getErrorMessage() const;
};

} // namespace sme::simulate
