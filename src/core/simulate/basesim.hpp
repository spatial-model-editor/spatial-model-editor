// Simulator interface class

#pragma once

#include <string>
#include <vector>

namespace sim {

struct IntegratorError {
  double abs = 0.0;
  double rel = 0.0;
};

class BaseSim {
 public:
  virtual ~BaseSim() = default;
  virtual void setIntegrationOrder(std::size_t order) = 0;
  virtual std::size_t getIntegrationOrder() const = 0;
  virtual void setIntegratorError(const IntegratorError& err) = 0;
  virtual IntegratorError getIntegratorError() const = 0;
  virtual void setMaxDt(double maxDt) = 0;
  virtual double getMaxDt() const = 0;
  virtual void setMaxThreads([[maybe_unused]] std::size_t maxThreads) = 0;
  virtual std::size_t getMaxThreads() const = 0;
  virtual std::size_t run(double time) = 0;
  virtual const std::vector<double>& getConcentrations(
      std::size_t compartmentIndex) const = 0;
  virtual double getLowerOrderConcentration(
      [[maybe_unused]] std::size_t compartmentIndex,
      [[maybe_unused]] std::size_t speciesIndex,
      [[maybe_unused]] std::size_t pixelIndex) const {
    return 0;
  }
  virtual std::string errorMessage() const { return {}; }
};

}  // namespace sim
