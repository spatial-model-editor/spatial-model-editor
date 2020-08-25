// Simulator interface class

#pragma once

#include <string>
#include <vector>

namespace simulate {

class BaseSim {
public:
  virtual ~BaseSim() = default;
  virtual std::size_t run(double time) = 0;
  virtual const std::vector<double> &
  getConcentrations(std::size_t compartmentIndex) const = 0;
  virtual std::string errorMessage() const { return {}; }
};

} // namespace simulate
