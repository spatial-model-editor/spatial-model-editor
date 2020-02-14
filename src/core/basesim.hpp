// Simulator interface class

#pragma once

#include <vector>

namespace sim {

class BaseSim {
 public:
  virtual ~BaseSim() = default;
  virtual void setIntegrationOrder(std::size_t order) = 0;
  virtual std::size_t run(double time, double relativeError,
                          double maximumStepsize) = 0;
  virtual const std::vector<double> &getConcentrations(
      std::size_t compartmentIndex) const = 0;
  virtual double getLowerOrderConcentration(
      [[maybe_unused]] std::size_t compartmentIndex,
      [[maybe_unused]] std::size_t speciesIndex,
      [[maybe_unused]] std::size_t pixelIndex) const {
    return 0;
  }
};

}  // namespace sim
