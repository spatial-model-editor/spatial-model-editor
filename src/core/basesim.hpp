// Simulator interface class

#pragma once

#include <vector>

namespace sim {

class BaseSim {
 public:
  virtual ~BaseSim() = default;
  virtual void doTimestep(double t, double dt) = 0;
  virtual const std::vector<double> &getConcentrations(
      std::size_t compartmentIndex) const = 0;
};

}  // namespace sim
