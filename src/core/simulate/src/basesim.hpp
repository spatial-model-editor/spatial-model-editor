// Simulator interface class

#pragma once

#include <QImage>
#include <string>
#include <vector>

namespace sme::simulate {

class BaseSim {
public:
  virtual ~BaseSim() = default;
  virtual std::size_t run(double time, double timeout_ms = -1.0) = 0;
  virtual const std::vector<double> &
  getConcentrations(std::size_t compartmentIndex) const = 0;
  virtual std::size_t getConcentrationPadding() const = 0;
  virtual const std::string &errorMessage() const = 0;
  virtual const QImage &errorImage() const = 0;
  virtual void setStopRequested(bool stop) = 0;
};

} // namespace sme::simulate
