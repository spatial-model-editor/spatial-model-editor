// Simulator interface class

#pragma once

#include "sme/image_stack.hpp"
#include <QImage>
#include <string>
#include <vector>

namespace sme::simulate {

class BaseSim {
public:
  virtual ~BaseSim() = default;
  virtual std::size_t run(double time, double timeout_ms,
                          const std::function<bool()> &stopRunningCallback) = 0;
  [[nodiscard]] virtual const std::vector<double> &
  getConcentrations(std::size_t compartmentIndex) const = 0;
  [[nodiscard]] virtual std::size_t getConcentrationPadding() const = 0;
  [[nodiscard]] virtual const std::string &errorMessage() const = 0;
  [[nodiscard]] virtual const common::ImageStack &errorImages() const = 0;
  virtual void setStopRequested(bool stop) = 0;
};

} // namespace sme::simulate
