// Simulator interface class

#pragma once

#include <string>
#include <vector>

namespace sme {

namespace simulate {

class BaseSim {
public:
  virtual ~BaseSim() = default;
  virtual std::size_t run(double time) = 0;
  virtual const std::vector<double> &
  getConcentrations(std::size_t compartmentIndex) const = 0;
  virtual std::size_t getConcentrationPadding() const = 0;
  virtual const std::string &errorMessage() const = 0;
  virtual void requestStop() = 0;
};

} // namespace simulate

} // namespace sme
